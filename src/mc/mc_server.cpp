/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <memory>
#include <system_error>

#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/signalfd.h>

#include <xbt/log.h>

#include "ModelChecker.hpp"
#include "mc_protocol.h"
#include "mc_server.h"
#include "mc_private.h"
#include "mc_ignore.h"
#include "mcer_ignore.h"

using simgrid::mc::remote;

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_server, mc, "MC server logic");

// HArdcoded index for now:
#define SOCKET_FD_INDEX 0
#define SIGNAL_FD_INDEX 1

mc_server_t mc_server;

struct mc_symbol_pointer_callback
{
  mc_process_t process;
  void* value;
};

static int mc_symbol_pointer_callback_evaluate(void* p)
{
  struct mc_symbol_pointer_callback* callback = (struct mc_symbol_pointer_callback*) p;
  int value;
  callback->process->read_bytes(&value, sizeof(value), remote(callback->value));
  return value;
}

s_mc_server::s_mc_server(pid_t pid, int socket)
{
  this->pid = pid;
  this->socket = socket;
}

void s_mc_server::start()
{
  /* Wait for the target process to initialize and exchange a HELLO messages
   * before trying to look at its memory map.
   */
  int res = MC_protocol_hello(socket);
  if (res != 0)
    throw std::system_error(res, std::system_category());

  // Block SIGCHLD (this will be handled with accept/signalfd):
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
    throw std::system_error(errno, std::system_category());

  sigset_t full_set;
  sigfillset(&full_set);

  // Prepare data for poll:

  struct pollfd* socket_pollfd = &fds[SOCKET_FD_INDEX];
  socket_pollfd->fd = socket;
  socket_pollfd->events = POLLIN;
  socket_pollfd->revents = 0;

  int signal_fd = signalfd(-1, &set, 0);
  if (signal_fd == -1)
    throw std::system_error(errno, std::system_category());

  struct pollfd* signalfd_pollfd = &fds[SIGNAL_FD_INDEX];
  signalfd_pollfd->fd = signal_fd;
  signalfd_pollfd->events = POLLIN;
  signalfd_pollfd->revents = 0;
}

void s_mc_server::shutdown()
{
  XBT_DEBUG("Shuting down model-checker");

  mc_process_t process = &mc_model_checker->process();
  int status = process->status;
  if (process->running) {
    XBT_DEBUG("Killing process");
    kill(process->pid, SIGTERM);
    if (waitpid(process->pid, &status, 0) == -1)
      throw std::system_error(errno, std::system_category());
    // TODO, handle the case when the process does not want to die with a timeout
    process->status = status;
  }
}

void s_mc_server::exit()
{
  // Finished:
  int status = mc_model_checker->process().status;
  if (WIFEXITED(status))
    ::exit(WEXITSTATUS(status));
  else if (WIFSIGNALED(status)) {
    // Try to uplicate the signal of the model-checked process.
    // This is a temporary hack so we don't try too hard.
    kill(mc_model_checker->process().pid, WTERMSIG(status));
    abort();
  } else {
    xbt_die("Unexpected status from model-checked process");
  }
}

void s_mc_server::resume(mc_process_t process)
{
  int socket = process->socket;
  int res = MC_protocol_send_simple_message(socket, MC_MESSAGE_CONTINUE);
  if (res)
    throw std::system_error(res, std::system_category());
  process->cache_flags = (mc_process_cache_flags_t) 0;
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

bool s_mc_server::handle_events()
{
  char buffer[MC_MESSAGE_LENGTH];
  struct pollfd* socket_pollfd = &fds[SOCKET_FD_INDEX];
  struct pollfd* signalfd_pollfd = &fds[SIGNAL_FD_INDEX];

  while(poll(fds, 2, -1) == -1) {
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
          MC_heap_region_ignore_remove(message.addr, message.size);
          break;
        }

      case MC_MESSAGE_IGNORE_MEMORY:
        {
          s_mc_ignore_memory_message_t message;
          if (size != sizeof(message))
            xbt_die("Broken messsage");
          memcpy(&message, buffer, sizeof(message));
          MC_process_ignore_memory(&mc_model_checker->process(),
            message.addr, message.size);
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
            xbt_die("Support for callbacks/functions symbols not implemented in client/server mode.");
          XBT_DEBUG("Received symbol: %s", message.name);

          struct mc_symbol_pointer_callback* callback = xbt_new(struct mc_symbol_pointer_callback, 1);
          callback->process = &mc_model_checker->process();
          callback->value   = message.data;

          MC_automaton_new_propositional_symbol_callback(message.name,
            mc_symbol_pointer_callback_evaluate, callback, free);
          break;
        }

      case MC_MESSAGE_WAITING:
        return false;

      case MC_MESSAGE_ASSERTION_FAILED:
        MC_report_assertion_error();
        xbt_abort();
        break;

      default:
        xbt_die("Unexpected message from model-checked application");

      }
      return true;
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

void s_mc_server::loop()
{
  while (mc_model_checker->process().running)
    this->handle_events();
}

void s_mc_server::handle_signals()
{
  struct signalfd_siginfo info;
  struct pollfd* signalfd_pollfd = &fds[SIGNAL_FD_INDEX];
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

void s_mc_server::handle_waitpid()
{
  XBT_DEBUG("Check for wait event");
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) != 0) {
    if (pid == -1) {
      if (errno == ECHILD) {
        // No more children:
        if (mc_model_checker->process().running)
          xbt_die("Inconsistent state");
        else
          break;
      } else {
        XBT_ERROR("Could not wait for pid");
        throw std::system_error(errno, std::system_category());
      }
    }

    if (pid == mc_model_checker->process().pid) {
      if (WIFEXITED(status) || WIFSIGNALED(status)) {
        XBT_DEBUG("Child process is over");
        mc_model_checker->process().status = status;
        mc_model_checker->process().running = false;
      }
    }
  }
}

void s_mc_server::on_signal(const struct signalfd_siginfo* info)
{
  switch(info->ssi_signo) {
  case SIGCHLD:
    this->handle_waitpid();
    break;
  default:
    break;
  }
}

void MC_server_wait_client(mc_process_t process)
{
  mc_server->resume(process);
  while (mc_model_checker->process().running) {
    if (!mc_server->handle_events())
      return;
  }
}

void MC_server_simcall_handle(mc_process_t process, unsigned long pid, int value)
{
  s_mc_simcall_handle_message m;
  memset(&m, 0, sizeof(m));
  m.type  = MC_MESSAGE_SIMCALL_HANDLE;
  m.pid   = pid;
  m.value = value;
  MC_protocol_send(mc_model_checker->process().socket, &m, sizeof(m));
  process->cache_flags = (mc_process_cache_flags_t) 0;
  while (mc_model_checker->process().running) {
    if (!mc_server->handle_events())
      return;
  }
}

void MC_server_loop(mc_server_t server)
{
  server->loop();
}

}
