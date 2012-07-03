  /* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "messages.h"
#include "bittorrent.h"
/**
 * Build a new empty message
 * @param type type of the message
 * @param issuer_host_name hostname of the issuer, for debuging purposes
 * @param mailbox mailbox where the peer should answer
 * @param peer_id id of the issuer
 */
msg_task_t task_message_new(e_message_type type, const char *issuer_host_name, const char *mailbox, int peer_id) {
  message_t message = xbt_new(s_message_t,1);
  message->issuer_host_name = issuer_host_name;
  message->peer_id = peer_id;
  message->mailbox = mailbox;
  message->type = type;
  msg_task_t task = MSG_task_create(NULL,0,MESSAGE_SIZE,message);
  return task;
}
/**
 * Builds a message containing an index.
 */
msg_task_t task_message_index_new(e_message_type type, const char *issuer_host_name, const char *mailbox, int peer_id, int index) {
  msg_task_t task = task_message_new(type,issuer_host_name, mailbox, peer_id);
  message_t message = MSG_task_get_data(task);
  message->index = index;
  return task;
}
msg_task_t task_message_bitfield_new(const char *issuer_host_name, const char *mailbox, int peer_id, char *bitfield) {
  msg_task_t task = task_message_new(MESSAGE_BITFIELD,issuer_host_name, mailbox, peer_id);
  message_t message = MSG_task_get_data(task);
  message->bitfield = bitfield;
  return task;
}
msg_task_t task_message_request_new(const char *issuer_host_name, const char *mailbox, int peer_id, int index, int block_index, int block_length) {
  msg_task_t task = task_message_index_new(MESSAGE_REQUEST,issuer_host_name, mailbox, peer_id, index);
  message_t message = MSG_task_get_data(task);
  message->block_index = block_index;
  message->block_length = block_length;
  return task;
}
msg_task_t task_message_piece_new(const char *issuer_host_name, const char *mailbox, int peer_id, int index, int stalled, int block_index, int block_length) {
  msg_task_t task = task_message_index_new(MESSAGE_PIECE,issuer_host_name, mailbox, peer_id, index);
  message_t message = MSG_task_get_data(task);
  message->stalled = stalled;
  message->block_index = block_index;
  message->block_length = block_length;
  return task;
}
void task_message_free(void* task) {
	message_t message = MSG_task_get_data(task);
	xbt_free(message);
	MSG_task_destroy(task);
}
