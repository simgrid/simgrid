/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/ModelChecker.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/LivenessChecker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/sosp/RemoteProcessMemory.hpp"
#include "src/mc/transition/TransitionComm.hpp"
#include "xbt/automaton.hpp"
#include "xbt/system_error.hpp"

#include <array>
#include <csignal>
#include <sys/ptrace.h>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_ModelChecker, mc, "ModelChecker");

::simgrid::mc::ModelChecker* mc_model_checker = nullptr;

#ifdef __linux__
# define WAITPID_CHECKED_FLAGS __WALL
#else
# define WAITPID_CHECKED_FLAGS 0
#endif

namespace simgrid::mc {

ModelChecker::ModelChecker(std::unique_ptr<RemoteProcessMemory> remote_memory, int sockfd)
    : checker_side_(sockfd), remote_process_memory_(std::move(remote_memory))
{
}

void ModelChecker::start()
{
  checker_side_.start(
      [](evutil_socket_t sig, short events, void* arg) {
        auto mc = static_cast<simgrid::mc::ModelChecker*>(arg);
        if (events == EV_READ) {
          std::array<char, MC_MESSAGE_LENGTH> buffer;
          ssize_t size = mc->checker_side_.get_channel().receive(buffer.data(), buffer.size(), false);
          if (size == -1 && errno != EAGAIN)
            throw simgrid::xbt::errno_error();

          if (not mc->handle_message(buffer.data(), size))
            mc->checker_side_.break_loop();
        } else if (events == EV_SIGNAL) {
          if (sig == SIGCHLD)
            mc->handle_waitpid();
          else
            xbt_die("Unexpected signal: %d", sig);
        } else {
          xbt_die("Unexpected event");
        }
      },
      this);

  XBT_DEBUG("Waiting for the model-checked process");
  int status;

  // The model-checked process SIGSTOP itself to signal it's ready:
  const pid_t pid = remote_process_memory_->pid();

  xbt_assert(waitpid(pid, &status, WAITPID_CHECKED_FLAGS) == pid && WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP,
             "Could not wait model-checked process");

  errno = 0;
#ifdef __linux__
  ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACEEXIT);
  ptrace(PTRACE_CONT, pid, 0, 0);
#elif defined BSD
  ptrace(PT_CONTINUE, pid, (caddr_t)1, 0);
#else
# error "no ptrace equivalent coded for this platform"
#endif
  xbt_assert(errno == 0,
             "Ptrace does not seem to be usable in your setup (errno: %d). "
             "If you run from within a docker, adding `--cap-add SYS_PTRACE` to the docker line may help. "
             "If it does not help, please report this bug.",
             errno);
}

bool ModelChecker::handle_message(const char* buffer, ssize_t size)
{
  s_mc_message_t base_message;
  xbt_assert(size >= (ssize_t)sizeof(base_message), "Broken message");
  memcpy(&base_message, buffer, sizeof(base_message));

  switch(base_message.type) {
    case MessageType::INITIAL_ADDRESSES: {
      s_mc_message_initial_addresses_t message;
      xbt_assert(size == sizeof(message), "Broken message. Got %d bytes instead of %d.", (int)size, (int)sizeof(message));
      memcpy(&message, buffer, sizeof(message));

      get_remote_process_memory().init(message.mmalloc_default_mdp);
      break;
    }

    case MessageType::IGNORE_HEAP: {
      s_mc_message_ignore_heap_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));

      IgnoredHeapRegion region;
      region.block    = message.block;
      region.fragment = message.fragment;
      region.address  = message.address;
      region.size     = message.size;
      get_remote_process_memory().ignore_heap(region);
      break;
    }

    case MessageType::UNIGNORE_HEAP: {
      s_mc_message_ignore_memory_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      get_remote_process_memory().unignore_heap((void*)(std::uintptr_t)message.addr, message.size);
      break;
    }

    case MessageType::IGNORE_MEMORY: {
      s_mc_message_ignore_memory_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      this->get_remote_process_memory().ignore_region(message.addr, message.size);
      break;
    }

    case MessageType::STACK_REGION: {
      s_mc_message_stack_region_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      this->get_remote_process_memory().stack_areas().push_back(message.stack_region);
    } break;

    case MessageType::REGISTER_SYMBOL: {
      s_mc_message_register_symbol_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      xbt_assert(not message.callback, "Support for client-side function proposition is not implemented.");
      XBT_DEBUG("Received symbol: %s", message.name.data());

      LivenessChecker::automaton_register_symbol(get_remote_process_memory(), message.name.data(),
                                                 remote((int*)message.data));
      break;
    }

    case MessageType::WAITING:
      return false;

    case MessageType::ASSERTION_FAILED:
      exploration_->report_assertion_failure();
      break;

    default:
      xbt_die("Unexpected message from model-checked application");
  }
  return true;
}

void ModelChecker::handle_waitpid()
{
  XBT_DEBUG("Check for wait event");
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) != 0) {
    if (pid == -1) {
      if (errno == ECHILD) {
        // No more children:
        xbt_assert(not this->get_remote_process_memory().running(), "Inconsistent state");
        break;
      } else {
        XBT_ERROR("Could not wait for pid");
        throw simgrid::xbt::errno_error();
      }
    }

    if (pid == this->get_remote_process_memory().pid()) {
      // From PTRACE_O_TRACEEXIT:
#ifdef __linux__
      if (status>>8 == (SIGTRAP | (PTRACE_EVENT_EXIT<<8))) {
        unsigned long eventmsg;
        xbt_assert(ptrace(PTRACE_GETEVENTMSG, get_remote_process_memory().pid(), 0, &eventmsg) != -1,
                   "Could not get exit status");
        status = static_cast<int>(eventmsg);
        if (WIFSIGNALED(status))
          exploration_->report_crash(status);
      }
#endif

      // We don't care about non-lethal signals, just reinject them:
      if (WIFSTOPPED(status)) {
        XBT_DEBUG("Stopped with signal %i", (int) WSTOPSIG(status));
        errno = 0;
#ifdef __linux__
        ptrace(PTRACE_CONT, get_remote_process_memory().pid(), 0, WSTOPSIG(status));
#elif defined BSD
        ptrace(PT_CONTINUE, get_remote_process_memory().pid(), (caddr_t)1, WSTOPSIG(status));
#endif
        xbt_assert(errno == 0, "Could not PTRACE_CONT");
      }

      else if (WIFSIGNALED(status)) {
        exploration_->report_crash(status);
      } else if (WIFEXITED(status)) {
        XBT_DEBUG("Child process is over");
        this->get_remote_process_memory().terminate();
      }
    }
  }
}

Transition* ModelChecker::handle_simcall(aid_t aid, int times_considered, bool new_transition)
{
  s_mc_message_simcall_execute_t m = {};
  m.type              = MessageType::SIMCALL_EXECUTE;
  m.aid_              = aid;
  m.times_considered_ = times_considered;
  checker_side_.get_channel().send(m);

  this->remote_process_memory_->clear_cache();
  if (this->remote_process_memory_->running())
    checker_side_.dispatch(); // The app may send messages while processing the transition

  s_mc_message_simcall_execute_answer_t answer;
  ssize_t s = checker_side_.get_channel().receive(answer);
  xbt_assert(s != -1, "Could not receive message");
  xbt_assert(s == sizeof answer, "Broken message (size=%zd; expected %zu)", s, sizeof answer);
  xbt_assert(answer.type == MessageType::SIMCALL_EXECUTE_ANSWER,
             "Received unexpected message %s (%i); expected MessageType::SIMCALL_EXECUTE_ANSWER (%i)",
             to_c_str(answer.type), (int)answer.type, (int)MessageType::SIMCALL_EXECUTE_ANSWER);

  if (new_transition) {
    std::stringstream stream(answer.buffer.data());
    return deserialize_transition(aid, times_considered, stream);
  } else
    return nullptr;
}

} // namespace simgrid::mc
