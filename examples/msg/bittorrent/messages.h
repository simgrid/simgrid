/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_MESSAGES_H_
#define BITTORRENT_MESSAGES_H_
#include <msg/msg.h>
/**
 * Types of messages exchanged between two peers.
 */
typedef enum {
  MESSAGE_HANDSHAKE,
  MESSAGE_CHOKE,
  MESSAGE_UNCHOKE,
  MESSAGE_INTERESTED,
  MESSAGE_NOTINTERESTED,
  MESSAGE_HAVE,
  MESSAGE_BITFIELD,
  MESSAGE_REQUEST,
  MESSAGE_PIECE
} e_message_type;

/**
 * Message data
 */
typedef struct s_message {
  e_message_type type;
  const char *mailbox;
  const char *issuer_host_name;
  int peer_id;
  char *bitfield;
  int index;
  int block_index;
  int block_length;
  int stalled:1;
} s_message_t, *message_t;
/**
 * Builds a new value-less message
 */
XBT_INLINE msg_task_t task_message_new(e_message_type type,
                                       const char *issuer_host_name,
                                       const char *mailbox, int peer_id);
/**
 * Builds a new "have/piece" message
 */
XBT_INLINE msg_task_t task_message_index_new(e_message_type type,
                                             const char *issuer_host_name,
                                             const char *mailbox, int peer_id,
                                             int index);
/**
 * Builds a new bitfield message
 */
msg_task_t task_message_bitfield_new(const char *issuer_host_name,
                                     const char *mailbox, int peer_id,
                                     char *bitfield);
/**
 * Builds a new "request" message
 */
msg_task_t task_message_request_new(const char *issuer_host_name,
                                    const char *mailbox, int peer_id, int index,
                                    int block_index, int block_length);

/**
 * Build a new "piece" message
 */
msg_task_t task_message_piece_new(const char *issuer_host_name,
                                  const char *mailbox, int peer_id, int index,
                                  int stalled, int block_index,
                                  int block_length);
/**
 * Free a message task
 */
void task_message_free(void *);
#endif                          /* BITTORRENT_MESSAGES_H_ */
