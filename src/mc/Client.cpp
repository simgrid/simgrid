/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>
#include <cerrno>

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/socket.h>

#include <xbt/log.h>
#include <xbt/sysdep.h>
#include <xbt/mmalloc.h>

#include "src/mc/mc_protocol.h"
#include "src/mc/Client.hpp"

// We won't need those once the separation MCer/MCed is complete:
#include "src/mc/mc_ignore.h"
#include "src/mc/mc_private.h" // MC_deadlock_check()
#include "src/mc/mc_smx.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_client, mc, "MC client logic");

namespace simgrid {
namespace mc {

std::unique_ptr<Client> Client::client_;

Client* Client::initialize()
{
  // We are not in MC mode:
  // TODO, handle this more gracefully.
  if (!getenv(MC_ENV_SOCKET_FD))
    return nullptr;

  // Do not break if we are called multiple times:
  if (client_)
    return client_.get();

  // Check and set the mode:
  if (mc_mode != MC_MODE_NONE)
    abort();
  mc_mode = MC_MODE_CLIENT;

  // Fetch socket from MC_ENV_SOCKET_FD:
  char* fd_env = std::getenv(MC_ENV_SOCKET_FD);
  if (!fd_env)
    xbt_die("No MC socket passed in the environment");
  int fd = xbt_str_parse_int(fd_env, bprintf("Variable %s should contain a number but contains '%%s'", MC_ENV_SOCKET_FD));
  XBT_DEBUG("Model-checked application found socket FD %i", fd);

  // Check the socket type/validity:
  int type;
  socklen_t socklen = sizeof(type);
  if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &socklen) != 0)
    xbt_die("Could not check socket type");
  if (type != SOCK_DGRAM)
    xbt_die("Unexpected socket type %i", type);
  XBT_DEBUG("Model-checked application found expected socket type");

  client_ = std::unique_ptr<Client>(new simgrid::mc::Client(fd));

  // Wait for the model-checker:
  if (ptrace(PTRACE_TRACEME, 0, nullptr, NULL) == -1 || raise(SIGSTOP) != 0)
    xbt_die("Could not wait for the model-checker");

  client_->handleMessages();
  return client_.get();
}

void Client::handleMessages()
{
  while (1) {
    XBT_DEBUG("Waiting messages from model-checker");

    char message_buffer[MC_MESSAGE_LENGTH];
    ssize_t s;

    if ((s = channel_.receive(&message_buffer, sizeof(message_buffer))) < 0)
      xbt_die("Could not receive commands from the model-checker");

    s_mc_message_t message;
    if ((size_t) s < sizeof(message))
      xbt_die("Received message is too small");
    memcpy(&message, message_buffer, sizeof(message));
    switch (message.type) {

    case MC_MESSAGE_DEADLOCK_CHECK:
      {
        int result = MC_deadlock_check();
        s_mc_int_message_t answer;
        answer.type = MC_MESSAGE_DEADLOCK_CHECK_REPLY;
        answer.value = result;
        if (channel_.send(answer))
          xbt_die("Could not send response");
      }
      break;

    case MC_MESSAGE_CONTINUE:
      return;

    case MC_MESSAGE_SIMCALL_HANDLE:
      {
        s_mc_simcall_handle_message_t message;
        if (s != sizeof(message))
          xbt_die("Unexpected size for SIMCALL_HANDLE");
        memcpy(&message, message_buffer, sizeof(message));
        smx_process_t process = SIMIX_process_from_PID(message.pid);
        if (!process)
          xbt_die("Invalid pid %lu", (unsigned long) message.pid);
        SIMIX_simcall_handle(&process->simcall, message.value);
        if (channel_.send(MC_MESSAGE_WAITING))
          xbt_die("Could not send MESSAGE_WAITING to model-checker");
      }
      break;

    case MC_MESSAGE_RESTORE:
      {
        s_mc_restore_message_t message;
        if (s != sizeof(message))
          xbt_die("Unexpected size for SIMCALL_HANDLE");
        memcpy(&message, message_buffer, sizeof(message));
        smpi_really_switch_data_segment(message.index);
      }
      break;

    default:
      xbt_die("%s received unexpected message %s (%i)",
        MC_mode_name(mc_mode),
        MC_message_type_name(message.type),
        message.type
      );
    }
  }
}

void Client::mainLoop(void)
{
  while (1) {
    if (channel_.send(MC_MESSAGE_WAITING))
      xbt_die("Could not send WAITING mesage to model-checker");
    this->handleMessages();
    simgrid::mc::wait_for_requests();
  }
}

}
}
