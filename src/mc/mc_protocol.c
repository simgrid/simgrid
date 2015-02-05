/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <xbt/log.h>

#include "mc_protocol.h"
#include "mc_client.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_protocol, mc, "Generic MC protocol logic");

int MC_protocol_send(int socket, void* message, size_t size)
{
  while (send(socket, message, size, 0) == -1) {
    if (errno == EINTR)
      continue;
    else
      return errno;
  }
  return 0;
}

int MC_protocol_send_simple_message(int socket, int type)
{
  s_mc_message_t message;
  message.type = type;
  return MC_protocol_send(socket, &message, sizeof(message));
}

int MC_protocol_hello(int socket)
{
  int e;
  if ((e = MC_protocol_send_simple_message(socket, MC_MESSAGE_HELLO)) != 0) {
    XBT_ERROR("Could not send HELLO message: %s", strerror(e));
    return 1;
  }

  s_mc_message_t message;
  message.type = MC_MESSAGE_NONE;

  size_t s;
  while ((s = recv(socket, &message, sizeof(message), 0)) == -1) {
    if (errno == EINTR)
      continue;
    else {
      XBT_ERROR("Could not receive HELLO message: %s", strerror(errno));
      return 2;
    }
  }
  if (s < sizeof(message) || message.type != MC_MESSAGE_HELLO) {
    XBT_ERROR("Did not receive suitable HELLO message. Who are you?");
    return 3;
  }

  return 0;
}
