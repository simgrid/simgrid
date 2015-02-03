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

#include "mc_protocol.h"
#include "mc_client.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_client, mc, "MC client logic");

typedef struct s_mc_client {
  int active;
  int fd;
} s_mc_client_t, mc_client_t;

static s_mc_client_t mc_client;

void MC_client_init(void)
{
  mc_client.fd = -1;
  mc_client.active = 1;
  char* fd_env = getenv(MC_ENV_SOCKET_FD);
  if (!fd_env)
    xbt_die("MC socket not found");

  int fd = atoi(fd_env);
  XBT_DEBUG("Model-checked application found socket FD %i", fd);

  int type;
  socklen_t socklen = sizeof(type);
  if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &socklen) != 0)
    xbt_die("Could not check socket type: %s", strerror(errno));
  if (type != SOCK_DGRAM)
    xbt_die("Unexpected socket type %i", type);
  XBT_DEBUG("Model-checked application found expected socket type");

  mc_client.fd = fd;
}

void MC_client_hello(void)
{
  XBT_DEBUG("Greeting the MC server");
  if (MC_protocol_hello(mc_client.fd) != 0)
    xbt_die("Could not say hello the MC server");
  XBT_DEBUG("Greeted the MC server");
}

void MC_client_handle_messages(void)
{
  while (1) {
    XBT_DEBUG("Waiting messages from model-checker");
    s_mc_message_t message;
    if (recv(mc_client.fd, &message, sizeof(message), 0) == -1)
      xbt_die("Could not receive commands from the model-checker: %s",
        strerror(errno));
    XBT_DEBUG("Receive message from model-checker");
    switch (message.type) {
    case MC_MESSAGE_CONTINUE:
      return;
    default:
      xbt_die("Unexpected message from model-checker %i", message.type);
    }
  }
}
