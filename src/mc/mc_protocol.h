/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PROTOCOL_H
#define SIMGRID_MC_PROTOCOL_H

#include <stdint.h>

#include <xbt/base.h>

#include "mc/datatypes.h"

SG_BEGIN_DECL()

// ***** Environment variables for passing context to the model-checked process

/** Environment variable name set by `simgrid-mc` to enable MC support in the
 *  children MC processes
 */
#define MC_ENV_VARIABLE "SIMGRID_MC"

/** Environment variable name used to pass the communication socket */
#define MC_ENV_SOCKET_FD "SIMGRID_MC_SOCKET_FD"

// ***** MC mode

typedef enum {
  MC_MODE_NONE = 0,
  MC_MODE_CLIENT,
  MC_MODE_SERVER
} e_mc_mode_t;

extern e_mc_mode_t mc_mode;

// ***** Messages

typedef enum {
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
  // MCer request to finish the restoration:
  MC_MESSAGE_RESTORE,
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

typedef struct s_mc_int_message {
  e_mc_message_type type;
  uint64_t value;
} s_mc_int_message_t, *mc_int_message_t;

typedef struct s_mc_ignore_heap_message {
  e_mc_message_type type;
  int block;
  int fragment;
  void *address;
  size_t size;
} s_mc_ignore_heap_message_t, *mc_ignore_heap_message_t;

typedef struct s_mc_ignore_memory_message {
  e_mc_message_type type;
  uint64_t addr;
  size_t size;
} s_mc_ignore_memory_message_t, *mc_ignore_memory_message_t;

typedef struct s_mc_stack_region_message {
  e_mc_message_type type;
  s_stack_region_t stack_region;
} s_mc_stack_region_message_t, *mc_stack_region_message_t;

typedef struct s_mc_simcall_handle_message {
  e_mc_message_type type;
  unsigned long pid;
  int value;
} s_mc_simcall_handle_message_t, *mc_simcall_handle_message;

typedef struct s_mc_register_symbol_message {
  e_mc_message_type type;
  char name[128];
  int (*callback)(void*);
  void* data;
} s_mc_register_symbol_message_t, * mc_register_symbol_message_t;

typedef struct s_mc_restore_message {
  e_mc_message_type type;
  int index;
} s_mc_restore_message_t, *mc_restore_message_t;

XBT_PRIVATE const char* MC_message_type_name(e_mc_message_type type);
XBT_PRIVATE const char* MC_mode_name(e_mc_mode_t mode);

SG_END_DECL()

#endif
