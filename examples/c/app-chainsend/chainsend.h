/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef CHAINSEND_H
#define CHAINSEND_H

#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "simgrid/instr.h"
#include "simgrid/mailbox.h"

#include "xbt/log.h"
#include "xbt/str.h"

#include <stdlib.h>

/* Connection parameters */
#define MAX_PENDING_COMMS 256
#define PIECE_SIZE 65536
#define MESSAGE_BUILD_CHAIN_SIZE 40
#define MESSAGE_SEND_DATA_HEADER_SIZE 1

/* Broadcaster struct */
typedef struct s_broadcaster {
  unsigned int host_count;
  unsigned int piece_count;
  sg_mailbox_t first;
  sg_mailbox_t* mailboxes;
  sg_comm_t* pending_sends;
} s_broadcaster_t;

typedef s_broadcaster_t* broadcaster_t;
typedef const s_broadcaster_t* const_broadcaster_t;
void broadcaster(int argc, char* argv[]);

/* Message struct */
typedef struct s_chain_message {
  sg_mailbox_t prev_;
  sg_mailbox_t next_;
  unsigned int num_pieces;
} s_chain_message_t;

typedef s_chain_message_t* chain_message_t;

/* Peer struct */
typedef struct s_peer {
  sg_mailbox_t prev;
  sg_mailbox_t next;
  sg_mailbox_t me;
  unsigned long long received_bytes;
  unsigned int received_pieces;
  unsigned int total_pieces;
  sg_comm_t* pending_recvs;
  sg_comm_t* pending_sends;
} s_peer_t;

typedef s_peer_t* peer_t;
void peer(int argc, char* argv[]);

#endif /* CHAINSEND_H */
