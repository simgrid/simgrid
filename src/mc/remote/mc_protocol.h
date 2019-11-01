/* Copyright (c) 2015-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PROTOCOL_H
#define SIMGRID_MC_PROTOCOL_H

#include "mc/datatypes.h"
#include "simgrid/forward.h"
#include "stdint.h"

SG_BEGIN_DECL

// ***** Environment variables for passing context to the model-checked process

/** Environment variable name set by `simgrid-mc` to enable MC support in the
 *  children MC processes
 */
#define MC_ENV_VARIABLE "SIMGRID_MC"

/** Environment variable name used to pass the communication socket */
#define MC_ENV_SOCKET_FD "SIMGRID_MC_SOCKET_FD"

// ***** Messages

enum e_mc_message_type {
  MC_MESSAGE_NONE,
  MC_MESSAGE_CONTINUE,
  MC_MESSAGE_IGNORE_HEAP,
  MC_MESSAGE_UNIGNORE_HEAP,
  MC_MESSAGE_IGNORE_MEMORY,
  MC_MESSAGE_STACK_REGION,
  MC_MESSAGE_REGISTER_SYMBOL,
  MC_MESSAGE_DEADLOCK_CHECK,
  MC_MESSAGE_DEADLOCK_CHECK_REPLY,
  MC_MESSAGE_WAITING,
  MC_MESSAGE_SIMCALL_HANDLE,
  MC_MESSAGE_ASSERTION_FAILED,
  MC_MESSAGE_ACTOR_ENABLED,
  MC_MESSAGE_ACTOR_ENABLED_REPLY
};

#define MC_MESSAGE_LENGTH 512

/** Basic structure for a MC message
 *
 *  The current version of the client/server protocol sends C structures over `AF_LOCAL`
 *  `SOCK_SEQPACKET` sockets. This means that the protocol is ABI/architecture specific:
 *  we currently can't model-check a x86 process from a x86_64 process.
 *
 *  Moreover the protocol is not stable. The same version of the library should be used
 *  for the client and the server.
 */

/* Basic structure: all message start with a message type */
struct s_mc_message_t {
  enum e_mc_message_type type;
};

struct s_mc_message_int_t {
  enum e_mc_message_type type;
  uint64_t value;
};

/* Client->Server */
struct s_mc_message_ignore_heap_t {
  enum e_mc_message_type type;
  int block;
  int fragment;
  void* address;
  size_t size;
};

struct s_mc_message_ignore_memory_t {
  enum e_mc_message_type type;
  uint64_t addr;
  size_t size;
};

struct s_mc_message_stack_region_t {
  enum e_mc_message_type type;
  s_stack_region_t stack_region;
};

struct s_mc_message_register_symbol_t {
  enum e_mc_message_type type;
  char name[128];
  int (*callback)(void*);
  void* data;
};

/* Server -> client */
struct s_mc_message_simcall_handle_t {
  enum e_mc_message_type type;
  unsigned long pid;
  int value;
};

struct s_mc_message_restore_t {
  enum e_mc_message_type type;
  int index;
};

struct s_mc_message_actor_enabled_t {
  enum e_mc_message_type type;
  aid_t aid; // actor ID
};

XBT_PRIVATE const char* MC_message_type_name(enum e_mc_message_type type);

SG_END_DECL

#endif
