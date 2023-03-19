/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/CheckerSide.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/LivenessChecker.hpp"
#include "src/mc/sosp/RemoteProcessMemory.hpp"
#include "xbt/system_error.hpp"

#include <csignal>
#include <sys/ptrace.h>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkerside, mc, "MC communication with the application");

namespace simgrid::mc {
CheckerSide::CheckerSide(int sockfd, pid_t pid) : channel_(sockfd), running_(true), pid_(pid)
{
  auto* base = event_base_new();
  base_.reset(base);

  auto* socket_event = event_new(
      base, get_channel().get_socket(), EV_READ | EV_PERSIST,
      [](evutil_socket_t sig, short events, void* arg) {
        auto checker = static_cast<simgrid::mc::CheckerSide*>(arg);
        if (events == EV_READ) {
          std::array<char, MC_MESSAGE_LENGTH> buffer;
          ssize_t size = recv(checker->get_channel().get_socket(), buffer.data(), buffer.size(), MSG_DONTWAIT);
          if (size == -1) {
            XBT_ERROR("Channel::receive failure: %s", strerror(errno));
            if (errno != EAGAIN)
              throw simgrid::xbt::errno_error();
          }

          if (not checker->handle_message(buffer.data(), size))
            checker->break_loop();
        } else {
          xbt_die("Unexpected event");
        }
      },
      this);
  event_add(socket_event, nullptr);
  socket_event_.reset(socket_event);

  auto* signal_event = event_new(
      base, SIGCHLD, EV_SIGNAL | EV_PERSIST,
      [](evutil_socket_t sig, short events, void* arg) {
        auto checker = static_cast<simgrid::mc::CheckerSide*>(arg);
        if (events == EV_SIGNAL) {
          if (sig == SIGCHLD)
            checker->handle_waitpid();
          else
            xbt_die("Unexpected signal: %d", sig);
        } else {
          xbt_die("Unexpected event");
        }
      },
      this);
  event_add(signal_event, nullptr);
  signal_event_.reset(signal_event);
}

void CheckerSide::dispatch_events() const
{
  event_base_dispatch(base_.get());
}

void CheckerSide::break_loop() const
{
  event_base_loopbreak(base_.get());
}

bool CheckerSide::handle_message(const char* buffer, ssize_t size)
{
  s_mc_message_t base_message;
  xbt_assert(size >= (ssize_t)sizeof(base_message), "Broken message");
  memcpy(&base_message, buffer, sizeof(base_message));

  switch (base_message.type) {
    case MessageType::INITIAL_ADDRESSES: {
      s_mc_message_initial_addresses_t message;
      xbt_assert(size == sizeof(message), "Broken message. Got %d bytes instead of %d.", (int)size,
                 (int)sizeof(message));
      memcpy(&message, buffer, sizeof(message));
      /* Create the memory address space, now that we have the mandatory information */
      remote_memory_ = std::make_unique<simgrid::mc::RemoteProcessMemory>(pid_, message.mmalloc_default_mdp);
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
      get_remote_memory().ignore_heap(region);
      break;
    }

    case MessageType::UNIGNORE_HEAP: {
      s_mc_message_ignore_memory_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      get_remote_memory().unignore_heap((void*)(std::uintptr_t)message.addr, message.size);
      break;
    }

    case MessageType::IGNORE_MEMORY: {
      s_mc_message_ignore_memory_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      get_remote_memory().ignore_region(message.addr, message.size);
      break;
    }

    case MessageType::STACK_REGION: {
      s_mc_message_stack_region_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      get_remote_memory().stack_areas().push_back(message.stack_region);
    } break;

    case MessageType::REGISTER_SYMBOL: {
      s_mc_message_register_symbol_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      xbt_assert(not message.callback, "Support for client-side function proposition is not implemented.");
      XBT_DEBUG("Received symbol: %s", message.name.data());

      LivenessChecker::automaton_register_symbol(get_remote_memory(), message.name.data(), remote((int*)message.data));
      break;
    }

    case MessageType::WAITING:
      return false;

    case MessageType::ASSERTION_FAILED:
      Exploration::get_instance()->report_assertion_failure();
      break;

    default:
      xbt_die("Unexpected message from model-checked application");
  }
  return true;
}

void CheckerSide::clear_memory_cache()
{
  if (remote_memory_)
    remote_memory_->clear_cache();
}

void CheckerSide::handle_waitpid()
{
  XBT_DEBUG("Check for wait event");
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) != 0) {
    if (pid == -1) {
      if (errno == ECHILD) { // No more children:
        xbt_assert(not this->running(), "Inconsistent state");
        break;
      } else {
        XBT_ERROR("Could not wait for pid");
        throw simgrid::xbt::errno_error();
      }
    }

    if (pid == get_pid()) {
      // From PTRACE_O_TRACEEXIT:
#ifdef __linux__
      if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXIT << 8))) {
        unsigned long eventmsg;
        xbt_assert(ptrace(PTRACE_GETEVENTMSG, pid, 0, &eventmsg) != -1, "Could not get exit status");
        status = static_cast<int>(eventmsg);
        if (WIFSIGNALED(status)) {
          this->terminate();
          Exploration::get_instance()->report_crash(status);
        }
      }
#endif

      // We don't care about non-lethal signals, just reinject them:
      if (WIFSTOPPED(status)) {
        XBT_DEBUG("Stopped with signal %i", (int)WSTOPSIG(status));
        errno = 0;
#ifdef __linux__
        ptrace(PTRACE_CONT, pid, 0, WSTOPSIG(status));
#elif defined BSD
        ptrace(PT_CONTINUE, pid, (caddr_t)1, WSTOPSIG(status));
#endif
        xbt_assert(errno == 0, "Could not PTRACE_CONT");
      }

      else if (WIFSIGNALED(status)) {
        this->terminate();
        Exploration::get_instance()->report_crash(status);
      } else if (WIFEXITED(status)) {
        XBT_DEBUG("Child process is over");
        this->terminate();
      }
    }
  }
}

} // namespace simgrid::mc
