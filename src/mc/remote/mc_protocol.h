/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PROTOCOL_H
#define SIMGRID_MC_PROTOCOL_H

// ***** Environment variables for passing context to the model-checked process

#ifdef __cplusplus

#include "src/kernel/actor/SimcallObserver.hpp"

#include "simgrid/forward.h" // aid_t
#include "src/mc/datatypes.h"
#include "src/xbt/mmalloc/mmalloc.h"
#include <xbt/utility.hpp>

#include <array>
#include <cstdint>

// ***** Messages
namespace simgrid::mc {

XBT_DECLARE_ENUM_CLASS(MessageType, NONE, INITIAL_ADDRESSES, CONTINUE, IGNORE_HEAP, UNIGNORE_HEAP, IGNORE_MEMORY,
                       STACK_REGION, REGISTER_SYMBOL, DEADLOCK_CHECK, DEADLOCK_CHECK_REPLY, WAITING, SIMCALL_EXECUTE,
                       SIMCALL_EXECUTE_ANSWER, ASSERTION_FAILED, ACTORS_STATUS, ACTORS_STATUS_REPLY, FINALIZE,
                       FINALIZE_REPLY);
} // namespace simgrid::mc

constexpr unsigned MC_MESSAGE_LENGTH                 = 512;
constexpr unsigned SIMCALL_SERIALIZATION_BUFFER_SIZE = 2048;

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
  simgrid::mc::MessageType type;
};

struct s_mc_message_int_t {
  simgrid::mc::MessageType type;
  uint64_t value;
};

/* Client->Server */
struct s_mc_message_initial_addresses_t {
  simgrid::mc::MessageType type;
  xbt_mheap_t mmalloc_default_mdp;
  unsigned long* maxpid;
};

struct s_mc_message_ignore_heap_t {
  simgrid::mc::MessageType type;
  int block;
  int fragment;
  void* address;
  size_t size;
};

struct s_mc_message_ignore_memory_t {
  simgrid::mc::MessageType type;
  uint64_t addr;
  size_t size;
};

struct s_mc_message_stack_region_t {
  simgrid::mc::MessageType type;
  s_stack_region_t stack_region;
};

struct s_mc_message_register_symbol_t {
  simgrid::mc::MessageType type;
  std::array<char, 128> name;
  int (*callback)(void*);
  void* data;
};

/* Server -> client */
struct s_mc_message_simcall_execute_t {
  simgrid::mc::MessageType type;
  aid_t aid_;
  int times_considered_;
};
struct s_mc_message_simcall_execute_answer_t {
  simgrid::mc::MessageType type;
  std::array<char, SIMCALL_SERIALIZATION_BUFFER_SIZE> buffer;
};

struct s_mc_message_restore_t {
  simgrid::mc::MessageType type;
  int index;
};

struct s_mc_message_actors_status_answer_t {
  simgrid::mc::MessageType type;
  int count;
  int transition_count; // The total number of transitions sent as a payload to the checker
};
struct s_mc_message_actors_status_one_t { // an array of `s_mc_message_actors_status_one_t[count]` is sent right after
                                          // after a `s_mc_message_actors_status_answer_t`
  aid_t aid;
  bool enabled;
  int max_considered;

  // The total number of transitions that are serialized and associated with this actor.
  // Enforced to be either `0` or the same as `max_considered`
  int n_transitions;
};

// Answer from an actor to the question "what are you about to run?"
struct s_mc_message_simcall_probe_one_t { // a series of `s_mc_message_simcall_probe_one_t`
                                          // is sent right after `s_mc_message_actors_status_one_t[]`
  std::array<char, SIMCALL_SERIALIZATION_BUFFER_SIZE> buffer;
};

#endif // __cplusplus
#endif
