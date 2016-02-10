/* Copyright (c) 2012, 2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "task.h"
#include "answer.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_kademlia_task, "Messages specific for this msg example");

/** @brief Creates a new "find node" task
  * @param sender_id the id of the node who sends the task
  * @param destination_id the id the sender is trying to find
  * @param hostname the hostname of the node, for logging purposes
  */
msg_task_t task_new_find_node(unsigned int sender_id, unsigned int destination_id, char *mailbox, const char *hostname)
{
  task_data_t data = xbt_new(s_task_data_t, 1);

  data->type = TASK_FIND_NODE;
  data->sender_id = sender_id;
  data->destination_id = destination_id;
  data->answer = NULL;
  data->answer_to = mailbox;
  data->issuer_host_name = hostname;

  msg_task_t task = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, data);

  return task;
}

/** @brief Creates a new "answer to find node" task
  *  @param sender_id the node who sent the task
  *  @param destination_id the node that should be found
  *  @param answer the answer to send
  *  @param mailbox The mailbox of the sender
  *  @param hostname sender hostname
  */
msg_task_t task_new_find_node_answer(unsigned int sender_id, unsigned int destination_id, answer_t answer,
                                     char *mailbox, const char *hostname)
{
  task_data_t data = xbt_new(s_task_data_t, 1);

  data->type = TASK_FIND_NODE_ANSWER;
  data->sender_id = sender_id;
  data->destination_id = destination_id;
  data->answer = answer;
  data->answer_to = mailbox;
  data->issuer_host_name = hostname;

  msg_task_t task = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, data);

  return task;
}

/** @brief Creates a new "ping" task
  * @param sender_id : sender node identifier
  * @param mailbox : mailbox where we should respond
  * @param hostname : hostname of the sender, for debugging purposes
  */
msg_task_t task_new_ping(unsigned int sender_id, char *mailbox, const char *hostname)
{
  task_data_t data = xbt_new(s_task_data_t, 1);

  data->type = TASK_PING;
  data->sender_id = sender_id;
  data->destination_id = 0;
  data->answer = NULL;
  data->answer_to = mailbox;
  data->issuer_host_name = hostname;

  msg_task_t task = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, data);

  return task;
}

/** @brief Creates a new "ping answer" task
  * @param sender_id : sender node identifier
  * @param mailbox : mailbox of the sender
  * @param hostname : hostname of the sender, for debugging purposes
  */
msg_task_t task_new_ping_answer(unsigned int sender_id, char *mailbox, const char *hostname)
{
  task_data_t data = xbt_new(s_task_data_t, 1);

  data->type = TASK_PING_ANSWER;
  data->sender_id = sender_id;
  data->destination_id = 0;
  data->answer = NULL;
  data->answer_to = mailbox;
  data->issuer_host_name = hostname;

  msg_task_t task = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, data);

  return task;
}

/** @brief Destroys a task and its data
  * @param task the task that'll be destroyed
  */
void task_free(msg_task_t task)
{
  xbt_assert((task != NULL), "Tried to free a NULL task");

  task_data_t data = MSG_task_get_data(task);

  if (data->answer) {
    answer_free(data->answer);
  }
  xbt_free(data);

  MSG_task_destroy(task);
}

/** @brief Destroys a task and its data (taking a void* pointer
  * @param task The task that'll be destroyed
  */
void task_free_v(void *task)
{
  task_free(task);
}
