/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>

#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/signalfd.h>
#include <sys/ptrace.h>

#include <memory>
#include <system_error>

#include <xbt/log.h>
#include <xbt/automaton.h>
#include <xbt/automaton.hpp>
#include <xbt/system_error.hpp>

#include "simgrid/sg_config.h"

#include "src/mc/ModelChecker.hpp"
#include "src/mc/PageStore.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/mc_protocol.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_ignore.h"
#include "src/mc/mc_exit.h"
#include "src/mc/mc_liveness.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_ModelChecker, mc, "ModelChecker");

::simgrid::mc::ModelChecker* mc_model_checker = nullptr;

using simgrid::mc::remote;

// Hardcoded index for now:
#define SOCKET_FD_INDEX 0
#define SIGNAL_FD_INDEX 1

namespace simgrid {
namespace mc {

ModelChecker::ModelChecker(std::unique_ptr<Process> process) :
  hostnames_(xbt_dict_new()),
  page_store_(500),
  process_(std::move(process)),
  parent_snapshot_(nullptr)
{

}

ModelChecker::~ModelChecker()
{
  xbt_dict_free(&this->hostnames_);
}

const char* ModelChecker::get_host_name(const char* hostname)
{
  // Lookup the host name in the dictionary (or create it):
  xbt_dictelm_t elt = xbt_dict_get_elm_or_null(this->hostnames_, hostname);
  if (!elt) {
    xbt_dict_set(this->hostnames_, hostname, nullptr, nullptr);
    elt = xbt_dict_get_elm_or_null(this->hostnames_, hostname);
    assert(elt);
  }
  return elt->key;
}

void ModelChecker::start()
{
  const pid_t pid = process_->pid();

  // Block SIGCHLD (this will be handled with accept/signalfd):
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1)
    throw simgrid::xbt::errno_error(errno);

  sigset_t full_set;
  sigfillset(&full_set);

  // Prepare data for poll:

  struct pollfd* socket_pollfd = &fds_[SOCKET_FD_INDEX];
  socket_pollfd->fd = process_->getChannel().getSocket();
  socket_pollfd->events = POLLIN;
  socket_pollfd->revents = 0;

  int signal_fd = signalfd(-1, &set, 0);
  if (signal_fd == -1)
    throw simgrid::xbt::errno_error(errno);

  struct pollfd* signalfd_pollfd = &fds_[SIGNAL_FD_INDEX];
  signalfd_pollfd->fd = signal_fd;
  signalfd_pollfd->events = POLLIN;
  signalfd_pollfd->revents = 0;

  XBT_DEBUG("Waiting for the model-checked process");
  int status;

  // The model-checked process SIGSTOP itself to signal it's ready:
  pid_t res = waitpid(pid, &status, __WALL);
  if (res < 0 || !WIFSTOPPED(status) || WSTOPSIG(status) != SIGSTOP)
    xbt_die("Could not wait model-checked process");

  process_->init();

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  if ((_sg_mc_dot_output_file != nullptr) && (_sg_mc_dot_output_file[0] != '\0'))
    MC_init_dot_output();

  setup_ignore();

  ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACEEXIT);
  ptrace(PTRACE_CONT, pid, 0, 0);
}

static const std::pair<const char*, const char*> ignored_local_variables[] = {
  std::pair<const char*, const char*>{  "e", "*" },
  std::pair<const char*, const char*>{ "__ex_cleanup", "*" },
  std::pair<const char*, const char*>{ "__ex_mctx_en", "*" },
  std::pair<const char*, const char*>{ "__ex_mctx_me", "*" },
  std::pair<const char*, const char*>{ "__xbt_ex_ctx_ptr", "*" },
  std::pair<const char*, const char*>{ "_log_ev", "*" },
  std::pair<const char*, const char*>{ "_throw_ctx", "*" },
  std::pair<const char*, const char*>{ "ctx", "*" },

  std::pair<const char*, const char*>{ "self", "simcall_BODY_mc_snapshot" },
  std::pair<const char*, const char*>{ "next_context", "smx_ctx_sysv_suspend_serial" },
  std::pair<const char*, const char*>{ "i", "smx_ctx_sysv_suspend_serial" },

  /* Ignore local variable about time used for tracing */
  std::pair<const char*, const char*>{ "start_time", "*" },
};

void ModelChecker::setup_ignore()
{
  Process& process = this->process();
  for (std::pair<const char*, const char*> const& var :
      ignored_local_variables)
    process.ignore_local_variable(var.first, var.second);

  /* Static variable used for tracing */
  process.ignore_global_variable("counter");

  /* SIMIX */
  process.ignore_global_variable("smx_total_comms");
}

void ModelChecker::shutdown()
{
  XBT_DEBUG("Shuting down model-checker");

  simgrid::mc::Process* process = &this->process();
  if (process->running()) {
    XBT_DEBUG("Killing process");
    kill(process->pid(), SIGTERM);
    process->terminate();
  }
}

void ModelChecker::resume(simgrid::mc::Process& process)
{
  int res = process.getChannel().send(MC_MESSAGE_CONTINUE);
  if (res)
    throw simgrid::xbt::errno_error(res);
  process.clear_cache();
}

static
void throw_socket_error(int fd)
{
  int error = 0;
  socklen_t errlen = sizeof(error);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == -1)
    error = errno;
  throw simgrid::xbt::errno_error(errno);
}

bool ModelChecker::handle_message(char* buffer, ssize_t size)
{
  s_mc_message_t base_message;
  if (size < (ssize_t) sizeof(base_message))
    xbt_die("Broken message");
  memcpy(&base_message, buffer, sizeof(base_message));

  switch(base_message.type) {

  case MC_MESSAGE_IGNORE_HEAP:
    {
      s_mc_ignore_heap_message_t message;
      if (size != sizeof(message))
        xbt_die("Broken messsage");
      memcpy(&message, buffer, sizeof(message));

      IgnoredHeapRegion region;
      region.block = message.block;
      region.fragment = message.fragment;
      region.address = message.address;
      region.size = message.size;
      process().ignore_heap(region);
      break;
    }

  case MC_MESSAGE_UNIGNORE_HEAP:
    {
      s_mc_ignore_memory_message_t message;
      if (size != sizeof(message))
        xbt_die("Broken messsage");
      memcpy(&message, buffer, sizeof(message));
      process().unignore_heap(
        (void *)(std::uintptr_t) message.addr, message.size);
      break;
    }

  case MC_MESSAGE_IGNORE_MEMORY:
    {
      s_mc_ignore_memory_message_t message;
      if (size != sizeof(message))
        xbt_die("Broken messsage");
      memcpy(&message, buffer, sizeof(message));
      this->process().ignore_region(message.addr, message.size);
      break;
    }

  case MC_MESSAGE_STACK_REGION:
    {
      s_mc_stack_region_message_t message;
      if (size != sizeof(message))
        xbt_die("Broken messsage");
      memcpy(&message, buffer, sizeof(message));
      this->process().stack_areas().push_back(message.stack_region);
    }
    break;

  case MC_MESSAGE_REGISTER_SYMBOL:
    {
      s_mc_register_symbol_message_t message;
      if (size != sizeof(message))
        xbt_die("Broken message");
      memcpy(&message, buffer, sizeof(message));
      if (message.callback)
        xbt_die("Support for client-side function proposition is not implemented.");
      XBT_DEBUG("Received symbol: %s", message.name);

      if (simgrid::mc::property_automaton == nullptr)
        simgrid::mc::property_automaton = xbt_automaton_new();

      simgrid::mc::Process* process = &this->process();
      simgrid::mc::RemotePtr<int> address
        = simgrid::mc::remote((int*) message.data);
      simgrid::xbt::add_proposition(simgrid::mc::property_automaton,
        message.name,
        [process, address]() { return process->read(address); }
        );

      break;
    }

  case MC_MESSAGE_WAITING:
    return false;

  case MC_MESSAGE_ASSERTION_FAILED:
    MC_report_assertion_error();
    this->exit(SIMGRID_MC_EXIT_SAFETY);
    break;

  default:
    xbt_die("Unexpected message from model-checked application");

  }
  return true;
}

/** Terminate the model-checker aplication */
void ModelChecker::exit(int status)
{
  // TODO, terminate the model checker politely instead of exiting rudel
  if (process().running())
    kill(process().pid(), SIGKILL);
  ::exit(status);
}

bool ModelChecker::handle_events()
{
  char buffer[MC_MESSAGE_LENGTH];
  struct pollfd* socket_pollfd = &fds_[SOCKET_FD_INDEX];
  struct pollfd* signalfd_pollfd = &fds_[SIGNAL_FD_INDEX];

  while(poll(fds_, 2, -1) == -1) {
    switch(errno) {
    case EINTR:
      continue;
    default:
      throw simgrid::xbt::errno_error(errno);
    }
  }

  if (socket_pollfd->revents) {
    if (socket_pollfd->revents & POLLIN) {
      ssize_t size = process_->getChannel().receive(buffer, sizeof(buffer), false);
      if (size == -1 && errno != EAGAIN)
        throw simgrid::xbt::errno_error(errno);
      return handle_message(buffer, size);
    }
    if (socket_pollfd->revents & POLLERR)
      throw_socket_error(socket_pollfd->fd);
    if (socket_pollfd->revents & POLLHUP)
      xbt_die("Socket hang up?");
  }

  if (signalfd_pollfd->revents) {
    if (signalfd_pollfd->revents & POLLIN) {
      this->handle_signals();
      return true;
    }
    if (signalfd_pollfd->revents & POLLERR)
      throw_socket_error(signalfd_pollfd->fd);
    if (signalfd_pollfd->revents & POLLHUP)
      xbt_die("Signalfd hang up?");
  }

  return true;
}

void ModelChecker::loop()
{
  while (this->process().running())
    this->handle_events();
}

void ModelChecker::handle_signals()
{
  struct signalfd_siginfo info;
  struct pollfd* signalfd_pollfd = &fds_[SIGNAL_FD_INDEX];
  while (1) {
    ssize_t size = read(signalfd_pollfd->fd, &info, sizeof(info));
    if (size == -1) {
      if (errno == EINTR)
        continue;
      else
        throw simgrid::xbt::errno_error(errno);
    } else if (size != sizeof(info))
        return throw std::runtime_error(
          "Bad communication with model-checked application");
    else
      break;
  }
  this->on_signal(&info);
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
        if (this->process().running())
          xbt_die("Inconsistent state");
        else
          break;
      } else {
        XBT_ERROR("Could not wait for pid");
        throw simgrid::xbt::errno_error(errno);
      }
    }

    if (pid == this->process().pid()) {

      // From PTRACE_O_TRACEEXIT:
      if (status>>8 == (SIGTRAP | (PTRACE_EVENT_EXIT<<8))) {
        if (ptrace(PTRACE_GETEVENTMSG, this->process().pid(), 0, &status) == -1)
          xbt_die("Could not get exit status");
        if (WIFSIGNALED(status)) {
          MC_report_crash(status);
          mc_model_checker->exit(SIMGRID_MC_EXIT_PROGRAM_CRASH);
        }
      }

      // We don't care about signals, just reinject them:
      if (WIFSTOPPED(status)) {
        XBT_DEBUG("Stopped with signal %i", (int) WSTOPSIG(status));
        if (ptrace(PTRACE_CONT, this->process().pid(), 0, WSTOPSIG(status)) == -1)
          xbt_die("Could not PTRACE_CONT");
      }

      else if (WIFEXITED(status) || WIFSIGNALED(status)) {
        XBT_DEBUG("Child process is over");
        this->process().terminate();
      }
    }
  }
}

void ModelChecker::on_signal(const struct signalfd_siginfo* info)
{
  switch(info->ssi_signo) {
  case SIGCHLD:
    this->handle_waitpid();
    break;
  default:
    break;
  }
}

void ModelChecker::wait_client(simgrid::mc::Process& process)
{
  this->resume(process);
  while (this->process().running())
    if (!this->handle_events())
      return;
}

void ModelChecker::simcall_handle(simgrid::mc::Process& process, unsigned long pid, int value)
{
  s_mc_simcall_handle_message m;
  memset(&m, 0, sizeof(m));
  m.type  = MC_MESSAGE_SIMCALL_HANDLE;
  m.pid   = pid;
  m.value = value;
  process.getChannel().send(m);
  process.clear_cache();
  while (process.running())
    if (!this->handle_events())
      return;
}

bool ModelChecker::checkDeadlock()
{
  int res;
  if ((res = this->process().getChannel().send(MC_MESSAGE_DEADLOCK_CHECK)))
    xbt_die("Could not check deadlock state");
  s_mc_int_message_t message;
  ssize_t s = mc_model_checker->process().getChannel().receive(message);
  if (s == -1)
    xbt_die("Could not receive message");
  if (s != sizeof(message) || message.type != MC_MESSAGE_DEADLOCK_CHECK_REPLY)
    xbt_die("%s received unexpected message %s (%i, size=%i) "
      "expected MC_MESSAGE_DEADLOCK_CHECK_REPLY (%i, size=%i)",
      MC_mode_name(mc_mode),
      MC_message_type_name(message.type), (int) message.type, (int) s,
      (int) MC_MESSAGE_DEADLOCK_CHECK_REPLY, (int) sizeof(message)
      );
  return message.value != 0;
}

}
}
