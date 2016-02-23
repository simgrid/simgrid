/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "messages.h"
#include "bittorrent.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_messages,
                             "Messages specific for the message factory");

#define BITS_TO_BYTES(x) ((x / 8) + (x % 8) ? 1 : 0)

/** @brief Build a new empty message
 * @param type type of the message
 * @param issuer_host_name hostname of the issuer, for debugging purposes
 * @param mailbox mailbox where the peer should answer
 * @param peer_id id of the issuer
 * @param size message size in bytes
 */
msg_task_t task_message_new(e_message_type type, const char *issuer_host_name, const char *mailbox, int peer_id,
                            int size)
{
  message_t message = xbt_new(s_message_t, 1);
  message->issuer_host_name = issuer_host_name;
  message->peer_id = peer_id;
  message->mailbox = mailbox;
  message->type = type;
  msg_task_t task = MSG_task_create(NULL, 0, size, message);
  XBT_DEBUG("type: %d size: %d", (int) type, size);
  return task;
}

/** Builds a message containing an index. */
msg_task_t task_message_index_new(e_message_type type, const char *issuer_host_name, const char *mailbox, int peer_id,
                                  int index, int varsize)
{
  msg_task_t task = task_message_new(type, issuer_host_name, mailbox, peer_id, task_message_size(type) + varsize);
  message_t message = MSG_task_get_data(task);
  message->index = index;
  return task;
}

msg_task_t task_message_bitfield_new(const char *issuer_host_name, const char *mailbox, int peer_id, char *bitfield,
                                     int bitfield_size)
{
  msg_task_t task = task_message_new(MESSAGE_BITFIELD, issuer_host_name, mailbox, peer_id,
                                     task_message_size(MESSAGE_BITFIELD) + BITS_TO_BYTES(bitfield_size));
  message_t message = MSG_task_get_data(task);
  message->bitfield = bitfield;
  return task;
}

msg_task_t task_message_request_new(const char *issuer_host_name, const char *mailbox, int peer_id, int index,
                                    int block_index, int block_length)
{
  msg_task_t task = task_message_index_new(MESSAGE_REQUEST, issuer_host_name, mailbox, peer_id, index, 0);
  message_t message = MSG_task_get_data(task);
  message->block_index = block_index;
  message->block_length = block_length;
  return task;
}

msg_task_t task_message_piece_new(const char *issuer_host_name, const char *mailbox, int peer_id, int index,
                                  int block_index, int block_length, int block_size)
{
  msg_task_t task = task_message_index_new(MESSAGE_PIECE, issuer_host_name, mailbox, peer_id, index,
                                           block_length * block_size);
  message_t message = MSG_task_get_data(task);
  message->block_index = block_index;
  message->block_length = block_length;
  return task;
}

void task_message_free(void *task)
{
  message_t message = MSG_task_get_data(task);
  xbt_free(message);
  MSG_task_destroy(task);
}

int task_message_size(e_message_type type)
{
  int size = 0;
  switch (type) {
  case MESSAGE_HANDSHAKE:
    size = MESSAGE_HANDSHAKE_SIZE;
    break;
  case MESSAGE_CHOKE:
    size = MESSAGE_CHOKE_SIZE;
    break;
  case MESSAGE_UNCHOKE:
    size = MESSAGE_UNCHOKE_SIZE;
    break;
  case MESSAGE_INTERESTED:
    size = MESSAGE_INTERESTED_SIZE;
    break;
  case MESSAGE_NOTINTERESTED:
    size = MESSAGE_INTERESTED_SIZE;
    break;
  case MESSAGE_HAVE:
    size = MESSAGE_HAVE_SIZE;
    break;
  case MESSAGE_BITFIELD:
    size = MESSAGE_BITFIELD_SIZE;
    break;
  case MESSAGE_REQUEST:
    size = MESSAGE_REQUEST_SIZE;
    break;
  case MESSAGE_PIECE:
    size = MESSAGE_PIECE_SIZE;
    break;
  case MESSAGE_CANCEL:
    size = MESSAGE_CANCEL_SIZE;
    break;
  }
  return size;
}
