/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <errno.h>
#include <error.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <xbt/log.h>
#include <xbt/sysdep.h>
#include <xbt/mmalloc.h>

#include "mc_protocol.h"
#include "mc_client.h"

// We won't need those once the separation MCer/MCed is complete:
#include "mc_mmalloc.h"
#include "mc_ignore.h"
#include "mc_model_checker.h"
#include "mc_private.h" // MC_deadlock_check()

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_client, mc, "MC client logic");

mc_client_t mc_client;

void MC_client_init(void)
{
  if (mc_client) {
    XBT_WARN("MC_client_init called more than once.");
    return;
  }

  char* fd_env = getenv(MC_ENV_SOCKET_FD);
  if (!fd_env)
    xbt_die("MC socket not found");

  int fd = atoi(fd_env);
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
}

void MC_client_hello(void)
{
  XBT_DEBUG("Greeting the MC server");
  if (MC_protocol_hello(mc_client->fd) != 0)
    xbt_die("Could not say hello the MC server");
  XBT_DEBUG("Greeted the MC server");
}

void MC_client_send_message(void* message, size_t size)
{
  if (MC_protocol_send(mc_client->fd, message, size))
    xbt_die("Could not send message %i", (int) ((mc_message_t)message)->type);
}

void MC_client_handle_messages(void)
{
  while (1) {
    XBT_DEBUG("Waiting messages from model-checker");

    char message_buffer[MC_MESSAGE_LENGTH];
    size_t s;
    if ((s = recv(mc_client->fd, &message_buffer, sizeof(message_buffer), 0)) == -1)
      xbt_die("Could not receive commands from the model-checker: %s",
        strerror(errno));

    XBT_DEBUG("Receive message from model-checker");
    s_mc_message_t message;
    if (s < sizeof(message))
      xbt_die("Message is too short");
    memcpy(&message, message_buffer, sizeof(message));
    switch (message.type) {

    case MC_MESSAGE_DEADLOCK_CHECK:
      {
        int result = MC_deadlock_check();
        s_mc_int_message_t answer;
        answer.type = MC_MESSAGE_DEADLOCK_CHECK_REPLY;
        answer.value = result;
        if (MC_protocol_send(mc_client->fd, &answer, sizeof(answer)))
          xbt_die("Could nor send response");
      }
      break;

    case MC_MESSAGE_CONTINUE:
      return;

    default:
      xbt_die("Unexpected message from model-checker %i", message.type);
    }
  }
}
