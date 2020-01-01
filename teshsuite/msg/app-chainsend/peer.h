/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef PEER_H
#define PEER_H

#include "simgrid/msg.h"
#include "xbt/dynar.h"

#include "common.h"
#include "messages.h"

#define PEER_SHUTDOWN_DEADLINE 60000

/* Peer struct */
typedef struct s_peer {
  int init;
  char* prev;
  char* next;
  char* me;
  int pieces;
  unsigned long long bytes;
  xbt_dynar_t pending_recvs;
  xbt_dynar_t pending_sends;
  unsigned int total_pieces;
} s_peer_t;
typedef s_peer_t* peer_t;

/* Peer: helper functions */
msg_error_t peer_wait_for_message(peer_t peer);
int peer_execute_task(peer_t peer, msg_task_t task);
void peer_init_chain(peer_t peer, const s_message_t* msg);
void peer_delete(peer_t p);
void peer_shutdown(const s_peer_t* p);
void peer_init(peer_t p, int argc, char* argv[]);
void peer_print_stats(const s_peer_t* p, float elapsed_time);

int peer(int argc, char* argv[]);

#endif /* PEER_H */
