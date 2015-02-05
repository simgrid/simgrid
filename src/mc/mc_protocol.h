/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_PROTOCOL_H
#define MC_PROTOCOL_H

#include <xbt/misc.h>

#include "mc/datatypes.h"

SG_BEGIN_DECL()

// ***** Environment variables for passing context to the model-checked process

/** Environment variable name set by `simgrid-mc` to enable MC support in the
 *  children MC processes
 */
#define MC_ENV_VARIABLE "SIMGRIC_MC"

/** Environment variable name used to pass the communication socket */
#define MC_ENV_SOCKET_FD "SIMGRID_MC_SOCKET_FD"

// ***** Messages

typedef enum {
  MC_MESSAGE_NONE = 0,
  MC_MESSAGE_HELLO = 1,
  MC_MESSAGE_CONTINUE = 2,
  MC_MESSAGE_IGNORE_REGION = 3,
} e_mc_message_type;

#define MC_MESSAGE_LENGTH 512

/** Basic structure for a MC message
 *
 *  The current version of the client/server protocol sends C structures over `AF_LOCAL`
 *  `SOCK_DGRAM` sockets. This means that the protocol is ABI/architecture specific:
 *  we currently can't model-check a x86 process from a x86_64 process.
 *
 *  Moreover the protocol is not stable. The same version of the library should be used
 *  for the client and the server.
 *
 *  This is the basic structure shared by all messages: all message start with a message
 *  type.
 */
typedef struct s_mc_message {
  e_mc_message_type type;
} s_mc_message_t, *mc_message_t;

typedef struct s_mc_ignore_region_message {
  e_mc_message_type type;
  s_mc_heap_ignore_region_t region;
} s_mc_ignore_region_message_t, *mc_ignore_region_message_t;

int MC_protocol_send(int socket, void* message, size_t size);
int MC_protocol_send_simple_message(int socket, int type);
int MC_protocol_hello(int socket);

SG_END_DECL()

#endif
