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
#include "src/mc/mc_client.h"

// We won't need those once the separation MCer/MCed is complete:
#include "src/mc/mc_ignore.h"
#include "src/mc/mc_private.h" // MC_deadlock_check()
#include "src/mc/mc_smx.h"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_client, mc, "MC client logic");

mc_client_t mc_client;

void MC_client_init(void)
{
  if (mc_mode != MC_MODE_NONE)
    return;
  if (!getenv(MC_ENV_SOCKET_FD))
    return;
  mc_mode = MC_MODE_CLIENT;

  if (mc_client) {
    XBT_WARN("MC_client_init called more than once.");
    return;
  }

  char* fd_env = std::getenv(MC_ENV_SOCKET_FD);
  if (!fd_env)
    xbt_die("MC socket not found");

  int fd = xbt_str_parse_int(fd_env,bprintf("Variable %s should contain a number but contains '%%s'", MC_ENV_SOCKET_FD));
  XBT_DEBUG("Model-checked application found socket FD %i", fd);

  int type;
  socklen_t socklen = sizeof(type);
  if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &socklen) != 0)
    xbt_die("Could not check socket type");
  if (type != SOCK_DGRAM)
    xbt_die("Unexpected socket type %i", type);
  XBT_DEBUG("Model-checked application found expected socket type");

  mc_client = xbt_new0(s_mc_client_t, 1);
  mc_client->fd = fd;
  mc_client->active = 1;

  // Waiting for the model-checker:
  if (ptrace(PTRACE_TRACEME, 0, nullptr, NULL) == -1 || raise(SIGSTOP) != 0)
    xbt_die("Could not wait for the model-checker");
  MC_client_handle_messages();
}

void MC_client_send_message(void* message, size_t size)
{
  if (MC_protocol_send(mc_client->fd, message, size))
    xbt_die("Could not send message %i", (int) ((mc_message_t)message)->type);
}

void MC_client_send_simple_message(e_mc_message_type type)
{
  if (MC_protocol_send_simple_message(mc_client->fd, type))
    xbt_die("Could not send message %i", type);
}

void MC_client_handle_messages(void)
{
  while (1) {
    XBT_DEBUG("Waiting messages from model-checker");

    char message_buffer[MC_MESSAGE_LENGTH];
    ssize_t s;
    if ((s = MC_receive_message(mc_client->fd, &message_buffer, sizeof(message_buffer), 0)) < 0)
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
        if (MC_protocol_send(mc_client->fd, &answer, sizeof(answer)))
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
        MC_protocol_send_simple_message(mc_client->fd, MC_MESSAGE_WAITING);
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

void MC_client_main_loop(void)
{
  while (1) {
    MC_protocol_send_simple_message(mc_client->fd, MC_MESSAGE_WAITING);
    MC_client_handle_messages();
    MC_wait_for_requests();
  }
}

}
