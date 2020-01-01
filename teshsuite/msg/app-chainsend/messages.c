/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "messages.h"

msg_task_t task_message_new(e_message_type type, unsigned int len)
{
  message_t msg      = xbt_new(s_message_t, 1);
  msg->type          = type;
  msg->prev_hostname = NULL;
  msg->next_hostname = NULL;
  msg_task_t task    = MSG_task_create(NULL, 0, len, msg);

  return task;
}

msg_task_t task_message_chain_new(const char* prev, const char* next, const unsigned int num_pieces)
{
  msg_task_t task    = task_message_new(MESSAGE_BUILD_CHAIN, MESSAGE_BUILD_CHAIN_SIZE);
  message_t msg      = MSG_task_get_data(task);
  msg->prev_hostname = xbt_strdup(prev);
  msg->next_hostname = xbt_strdup(next);
  msg->num_pieces    = num_pieces;

  return task;
}

msg_task_t task_message_data_new(const char* block, unsigned int len)
{
  msg_task_t task  = task_message_new(MESSAGE_SEND_DATA, MESSAGE_SEND_DATA_HEADER_SIZE + len);
  message_t msg    = MSG_task_get_data(task);
  msg->data_block  = block;
  msg->data_length = len;

  return task;
}

void task_message_delete(void* task)
{
  message_t msg = MSG_task_get_data(task);
  xbt_free(msg);
  MSG_task_destroy(task);
}
