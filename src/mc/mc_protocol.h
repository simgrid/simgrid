/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_PROTOCOL_H
#define MC_PROTOCOL_H

#include <xbt/misc.h>

SG_BEGIN_DECL()

// ***** Environment variables for passing context to the model-checked process

/** Environment variable name set by `simgrid-mc` to enable MC support in the
 *  children MC processes
 */
#define MC_ENV_VARIABLE "SIMGRIC_MC"

/** Environment variable name used to pass the communication socket */
#define MC_ENV_SOCKET_FD "SIMGRID_MC_SOCKET_FD"

// ***** Messages

enum {
  MC_MESSAGE_NONE = 0,
  MC_MESSAGE_HELLO = 1,
  MC_MESSAGE_CONTINUE = 2,
};

typedef struct s_mc_message {
  int type;
} s_mc_message_t, *mc_message_t;

/**
 *  @return errno
 */
int MC_protocol_send_simple_message(int socket, int type);
int MC_protocol_hello(int socket);

SG_END_DECL()

#endif
