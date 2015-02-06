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

#include "mc_model_checker.h"
#include "mc_protocol.h"
#include "mc_server.h"
#include "mc_private.h"
#include "mc_ignore.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_server, mc, "MC server logic");

// HArdcoded index for now:
#define SOCKET_FD_INDEX 0
#define SIGNAL_FD_INDEX 1

mc_server_t mc_server;

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
  XBT_DEBUG("Greeting the MC client");
  int res = MC_protocol_hello(socket);
  if (res != 0)
    throw std::system_error(res, std::system_category());
  XBT_DEBUG("Greeted the MC client");

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

  mc_process_t process = &mc_model_checker->process;
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
  int status = mc_model_checker->process.status;
  if (WIFEXITED(status))
    ::exit(WEXITSTATUS(status));
  else if (WIFSIGNALED(status)) {
    // Try to uplicate the signal of the model-checked process.
    // This is a temporary hack so we don't try too hard.
    kill(mc_model_checker->process.pid, WTERMSIG(status));
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

void s_mc_server::handle_events()
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

      ssize_t size = recv(socket_pollfd->fd, buffer, sizeof(buffer), MSG_DONTWAIT);
      if (size == -1 && errno != EAGAIN)
        throw std::system_error(errno, std::system_category());

      s_mc_message_t base_message;
      if (size < (ssize_t) sizeof(base_message))
        xbt_die("Broken message");
      memcpy(&base_message, buffer, sizeof(base_message));

      switch(base_message.type) {

      case MC_MESSAGE_IGNORE_REGION:
        XBT_DEBUG("Received ignored region");
        if (size != sizeof(s_mc_ignore_region_message_t))
          xbt_die("Broken messsage");
        // TODO, handle the message
        break;

      case MC_MESSAGE_IGNORE_MEMORY:
        {
          XBT_DEBUG("Received ignored memory");
          if (size != sizeof(s_mc_ignore_memory_message_t))
            xbt_die("Broken messsage");
          s_mc_ignore_memory_message_t message;
          memcpy(&message, buffer, sizeof(message));
          MC_process_ignore_memory(&mc_model_checker->process,
            message.addr, message.size);
        }
        break;

      default:
        xbt_die("Unexpected message from model-checked application");

      }
      return;
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
      return;
    }
    if (signalfd_pollfd->revents & POLLERR) {
      throw_socket_error(signalfd_pollfd->fd);
    }
    if (signalfd_pollfd->revents & POLLHUP)
      xbt_die("Signalfd hang up?");
  }
}

void s_mc_server::loop()
{
  while (mc_model_checker->process.running)
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
        if (mc_model_checker->process.running)
          xbt_die("Inconsistent state");
        else
          break;
      } else {
        XBT_ERROR("Could not wait for pid: %s", strerror(errno));
        throw std::system_error(errno, std::system_category());
      }
    }

    if (pid == mc_model_checker->process.pid) {
      if (WIFEXITED(status) || WIFSIGNALED(status)) {
        XBT_DEBUG("Child process is over");
        mc_model_checker->process.status = status;
        mc_model_checker->process.running = false;
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
