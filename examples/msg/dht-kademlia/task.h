/* Copyright (c) 2012, 2014-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _MSG_KADEMLIA_EXAMPLES_TASK
#define _MSG_KADEMLIA_EXAMPLES_TASK
#include "common.h"
#include "node.h"
#include "simgrid/msg.h"

/* Types of tasks exchanged */
typedef enum {
  TASK_FIND_NODE,
  TASK_FIND_NODE_ANSWER,
  TASK_FIND_VALUE,
  TASK_FIND_VALUE_ANSWER,
  TASK_PING,
  TASK_PING_ANSWER,
  TASK_LEAVING
} e_task_type_t;

/* Data attached with the tasks */
typedef struct s_task_data {
  e_task_type_t type;
  unsigned int sender_id;       //Id of the guy who sent the task
  unsigned int destination_id;  //Id we are trying to find, if needed.
  answer_t answer;              //Answer to the request made, if needed.
  char *answer_to;              // mailbox to send the answer to (if not an answer).
  const char *issuer_host_name; // used for logging
} s_task_data_t, *task_data_t;

//Task handling functions
msg_task_t task_new_find_node(unsigned int sender_id, unsigned int destination_id, char *mailbox, const char *hostname);
msg_task_t task_new_find_node_answer(unsigned int sender_id, unsigned int destination_id, answer_t answer,
                                     char *mailbox, const char *hostname);
msg_task_t task_new_ping(unsigned int sender_id, char *mailbox, const char *hostname);
msg_task_t task_new_ping_answer(unsigned int sender_id, char *mailbox, const char *hostname);
void task_free(msg_task_t task);
void task_free_v(void *task);
#endif                          /* _MSG_KADEMLIA_EXAMPLES_TASK */
