/* Copyright (c) 2012-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_MESSAGES_H_
#define BITTORRENT_MESSAGES_H_
#include <simgrid/msg.h>

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
  const char *mailbox;
  const char *issuer_host_name;
  int peer_id;
  char *bitfield;
  int index;
  int block_index;
  int block_length;
} s_message_t, *message_t;

/** Builds a new value-less message */
msg_task_t task_message_new(e_message_type type, const char *issuer_host_name, const char *mailbox, int peer_id,
                            int size);
/** Builds a new "have/piece" message */
msg_task_t task_message_index_new(e_message_type type, const char *issuer_host_name, const char *mailbox, int peer_id,
                                  int index, int varsize);
/** Builds a new bitfield message */
msg_task_t task_message_bitfield_new(const char *issuer_host_name, const char *mailbox, int peer_id, char *bitfield,
                                     int bitfield_size);
/** Builds a new "request" message */
msg_task_t task_message_request_new(const char *issuer_host_name, const char *mailbox, int peer_id, int index,
                                    int block_index, int block_length);
/** Build a new "piece" message */
msg_task_t task_message_piece_new(const char *issuer_host_name, const char *mailbox, int peer_id, int index,
                                  int block_index, int block_length, int block_size);
/** Free a message task */
void task_message_free(void *);
int task_message_size(e_message_type type);

#endif                          /* BITTORRENT_MESSAGES_H_ */
