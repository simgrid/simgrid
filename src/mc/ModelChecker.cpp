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

#include "simgrid/sg_config.h"

#include "ModelChecker.hpp"
#include "PageStore.hpp"
#include "ModelChecker.hpp"
#include "mc_protocol.h"
#include "mc_private.h"
#include "mc_ignore.h"
#include "mcer_ignore.h"
#include "mc_exit.h"
#include "src/mc/mc_liveness.h"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_ModelChecker, mc, "ModelChecker");

}

::simgrid::mc::ModelChecker* mc_model_checker = nullptr;

using simgrid::mc::remote;

// Hardcoded index for now:
#define SOCKET_FD_INDEX 0
#define SIGNAL_FD_INDEX 1

namespace simgrid {
namespace mc {

ModelChecker::ModelChecker(pid_t pid, int socket) :
  pid_(pid), socket_(socket),
  hostnames_(xbt_dict_new()),
  page_store_(500),
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
  // Block SIGCHLD (this will be handled with accept/signalfd):
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1)
    throw std::system_error(errno, std::system_category());

  sigset_t full_set;
  sigfillset(&full_set);

  // Prepare data for poll:

  struct pollfd* socket_pollfd = &fds_[SOCKET_FD_INDEX];
  socket_pollfd->fd = socket_;
  socket_pollfd->events = POLLIN;
  socket_pollfd->revents = 0;

  int signal_fd = signalfd(-1, &set, 0);
  if (signal_fd == -1)
    throw std::system_error(errno, std::system_category());

  struct pollfd* signalfd_pollfd = &fds_[SIGNAL_FD_INDEX];
  signalfd_pollfd->fd = signal_fd;
  signalfd_pollfd->events = POLLIN;
  signalfd_pollfd->revents = 0;

  XBT_DEBUG("Waiting for the model-checked process");
  int status;

  // The model-checked process SIGSTOP itself to signal it's ready:
  pid_t res = waitpid(pid_, &status, __WALL);
  if (res < 0 || !WIFSTOPPED(status) || WSTOPSIG(status) != SIGSTOP)
    xbt_die("Could not wait model-checked process");

  assert(process_ == nullptr);
  process_ = std::unique_ptr<Process>(new Process(pid_, socket_));
  // TODO, avoid direct dependency on sg_cfg
  process_->privatized(sg_cfg_get_boolean("smpi/privatize_global_variables"));

  mc_comp_times = xbt_new0(s_mc_comparison_times_t, 1);

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  if ((_sg_mc_dot_output_file != nullptr) && (_sg_mc_dot_output_file[0] != '\0'))
    MC_init_dot_output();

  /* Init parmap */
  //parmap = xbt_parmap_mc_new(xbt_os_get_numcores(), XBT_PARMAP_DEFAULT);

  setup_ignore();

  ptrace(PTRACE_SETOPTIONS, pid_, nullptr, PTRACE_O_TRACEEXIT);
  ptrace(PTRACE_CONT, pid_, 0, 0);
}

void ModelChecker::setup_ignore()
{
  /* Ignore some variables from xbt/ex.h used by exception e for stacks comparison */
  MC_ignore_local_variable("e", "*");
  MC_ignore_local_variable("__ex_cleanup", "*");
  MC_ignore_local_variable("__ex_mctx_en", "*");
  MC_ignore_local_variable("__ex_mctx_me", "*");
  MC_ignore_local_variable("__xbt_ex_ctx_ptr", "*");
  MC_ignore_local_variable("_log_ev", "*");
  MC_ignore_local_variable("_throw_ctx", "*");
  MC_ignore_local_variable("ctx", "*");

  MC_ignore_local_variable("self", "simcall_BODY_mc_snapshot");
  MC_ignore_local_variable("next_cont"
    "ext", "smx_ctx_sysv_suspend_serial");
  MC_ignore_local_variable("i", "smx_ctx_sysv_suspend_serial");

  /* Ignore local variable about time used for tracing */
  MC_ignore_local_variable("start_time", "*");

  /* Static variable used for tracing */
  MCer_ignore_global_variable("counter");

  /* SIMIX */
  MCer_ignore_global_variable("smx_total_comms");
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
  int res = process.send_message(MC_MESSAGE_CONTINUE);
  if (res)
    throw std::system_error(res, std::system_category());
  process.cache_flags = (mc_process_cache_flags_t) 0;
}

static
void throw_socket_error(int fd)
{
  int error = 0;
  socklen_t errlen = sizeof(error);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == -1)
    error = errno;
  throw std::system_error(error, std::system_category());
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
      mc_heap_ignore_region_t region = xbt_new(s_mc_heap_ignore_region_t, 1);
      *region = message.region;
      MC_heap_region_ignore_insert(region);
      break;
    }

  case MC_MESSAGE_UNIGNORE_HEAP:
    {
      s_mc_ignore_memory_message_t message;
      if (size != sizeof(message))
        xbt_die("Broken messsage");
      memcpy(&message, buffer, sizeof(message));
      MC_heap_region_ignore_remove(
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
      stack_region_t stack_region = xbt_new(s_stack_region_t, 1);
      *stack_region = message.stack_region;
      MC_stack_area_add(stack_region);
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

      if (_mc_property_automaton == nullptr)
        _mc_property_automaton = xbt_automaton_new();

      simgrid::mc::Process* process = &this->process();
      simgrid::mc::remote_ptr<int> address
        = simgrid::mc::remote((int*) message.data);
      simgrid::xbt::add_proposition(_mc_property_automaton,
        message.name,
        [process, address]() { return process->read(address); }
        );

      break;
    }

  case MC_MESSAGE_WAITING:
    return false;

  case MC_MESSAGE_ASSERTION_FAILED:
    MC_report_assertion_error();
    ::exit(SIMGRID_MC_EXIT_SAFETY);
    break;

  default:
    xbt_die("Unexpected message from model-checked application");

  }
  return true;
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
      throw std::system_error(errno, std::system_category());
    }
  }

  if (socket_pollfd->revents) {
    if (socket_pollfd->revents & POLLIN) {
      ssize_t size = MC_receive_message(socket_pollfd->fd, buffer, sizeof(buffer), MSG_DONTWAIT);
      if (size == -1 && errno != EAGAIN)
        throw std::system_error(errno, std::system_category());
      return handle_message(buffer, size);
    }
    if (socket_pollfd->revents & POLLERR) {
      throw_socket_error(socket_pollfd->fd);
    }
    if (socket_pollfd->revents & POLLHUP)
      xbt_die("Socket hang up?");
  }

  if (signalfd_pollfd->revents) {
    if (signalfd_pollfd->revents & POLLIN) {
      this->handle_signals();
      return true;
    }
    if (signalfd_pollfd->revents & POLLERR) {
      throw_socket_error(signalfd_pollfd->fd);
    }
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
        throw std::system_error(errno, std::system_category());
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
        throw std::system_error(errno, std::system_category());
      }
    }

    if (pid == this->process().pid()) {

      // From PTRACE_O_TRACEEXIT:
      if (status>>8 == (SIGTRAP | (PTRACE_EVENT_EXIT<<8))) {
        if (ptrace(PTRACE_GETEVENTMSG, pid_, 0, &status) == -1)
          xbt_die("Could not get exit status");
        if (WIFSIGNALED(status)) {
          MC_report_crash(status);
          ::exit(SIMGRID_MC_EXIT_PROGRAM_CRASH);
        }
      }

      // We don't care about signals, just reinject them:
      if (WIFSTOPPED(status)) {
        XBT_DEBUG("Stopped with signal %i", (int) WSTOPSIG(status));
        if (ptrace(PTRACE_CONT, pid_, 0, WSTOPSIG(status)) == -1)
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
  while (this->process().running()) {
    if (!this->handle_events())
      return;
  }
}

void ModelChecker::simcall_handle(simgrid::mc::Process& process, unsigned long pid, int value)
{
  s_mc_simcall_handle_message m;
  memset(&m, 0, sizeof(m));
  m.type  = MC_MESSAGE_SIMCALL_HANDLE;
  m.pid   = pid;
  m.value = value;
  process.send_message(m);
  process.cache_flags = (mc_process_cache_flags_t) 0;
  while (process.running()) {
    if (!this->handle_events())
      return;
  }
}

}
}
