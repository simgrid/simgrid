/* Copyright (c) 2012-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_BITTORRENT_H_
#define BITTORRENT_BITTORRENT_H_
#include <simgrid/actor.h>
#include <simgrid/engine.h>
#include <simgrid/host.h>
#include <simgrid/mailbox.h>
#include <xbt/log.h>
#include <xbt/str.h>
#include <xbt/sysdep.h>

#define MAILBOX_SIZE 40
#define TRACKER_MAILBOX "tracker_mailbox"
/** Max number of pairs sent by the tracker to clients */
#define MAXIMUM_PEERS 50
/** Interval of time where the peer should send a request to the tracker */
#define TRACKER_QUERY_INTERVAL 1000
/** Communication size for a task to the tracker */
#define TRACKER_COMM_SIZE 1
#define GET_PEERS_TIMEOUT 10000
/** Number of peers that can be unchocked at a given time */
#define MAX_UNCHOKED_PEERS 4
/** Interval between each update of the choked peers */
#define UPDATE_CHOKED_INTERVAL 30

/** Message sizes
 * Sizes based on report by A. Legout et al, Understanding BitTorrent: An Experimental Perspective
 * http://hal.inria.fr/inria-00000156/en
 */
#define MESSAGE_HANDSHAKE_SIZE 68
#define MESSAGE_CHOKE_SIZE 5
#define MESSAGE_UNCHOKE_SIZE 5
#define MESSAGE_INTERESTED_SIZE 5
#define MESSAGE_NOTINTERESTED_SIZE 5
#define MESSAGE_HAVE_SIZE 9
#define MESSAGE_BITFIELD_SIZE 5
#define MESSAGE_REQUEST_SIZE 17
#define MESSAGE_PIECE_SIZE 13
#define MESSAGE_CANCEL_SIZE 17

/** Types of messages exchanged between two peers. */
typedef enum {
  MESSAGE_HANDSHAKE,
  MESSAGE_CHOKE,
  MESSAGE_UNCHOKE,
  MESSAGE_INTERESTED,
  MESSAGE_NOTINTERESTED,
  MESSAGE_HAVE,
  MESSAGE_BITFIELD,
  MESSAGE_REQUEST,
  MESSAGE_PIECE,
  MESSAGE_CANCEL
} e_message_type;

/** Message data */
typedef struct s_message {
  e_message_type type;
  int peer_id;
  sg_mailbox_t return_mailbox;
  unsigned int bitfield;
  int piece;
  int block_index;
  int block_length;
} s_message_t;
typedef s_message_t* message_t;

/** Builds a new value-less message */
message_t message_new(e_message_type type, int peer_id, sg_mailbox_t return_mailbox);
/** Builds a new "have/piece" message */
message_t message_index_new(e_message_type type, int peer_id, sg_mailbox_t return_mailbox, int index);
message_t message_other_new(e_message_type type, int peer_id, sg_mailbox_t return_mailbox, unsigned int bitfield);
/** Builds a new "request" message */
message_t message_request_new(int peer_id, sg_mailbox_t return_mailbox, int piece, int block_index, int block_length);
/** Build a new "piece" message */
message_t message_piece_new(int peer_id, sg_mailbox_t return_mailbox, int index, int block_index, int block_length);

#endif /* BITTORRENT_BITTORRENT_H_ */
