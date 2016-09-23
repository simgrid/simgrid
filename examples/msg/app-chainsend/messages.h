/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MESSAGES_H
#define MESSAGES_H

#include "simgrid/msg.h"

#define MESSAGE_BUILD_CHAIN_SIZE 40
#define MESSAGE_SEND_DATA_HEADER_SIZE 1
#define MESSAGE_END_DATA_SIZE 1

/* Messages enum */
typedef enum {
  MESSAGE_BUILD_CHAIN = 0,
  MESSAGE_SEND_DATA
} e_message_type;

/* Message struct */
typedef struct s_message {
  e_message_type type;
  char *prev_hostname;
  char *next_hostname;
  const char *data_block;
  unsigned int data_length;
  unsigned int num_pieces;
} s_message_t, *message_t;

/* Message methods */
msg_task_t task_message_new(e_message_type type, unsigned int len);
msg_task_t task_message_chain_new(const char* prev, const char *next, const unsigned int num_pieces);
msg_task_t task_message_data_new(const char *block, unsigned int len);
void task_message_delete(void *);

#endif /* MESSAGES_H */
