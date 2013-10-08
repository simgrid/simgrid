#include "messages.h"

msg_task_t task_message_new(e_message_type type, unsigned int len, const char *issuer_hostname, const char *mailbox)
{
  message_t msg = xbt_new(s_message_t, 1);
  msg->type = type;
  msg->issuer_hostname = xbt_strdup(issuer_hostname);
  msg->mailbox = xbt_strdup(mailbox);
  msg->prev_hostname = NULL;
  msg->next_hostname = NULL;
  msg_task_t task = MSG_task_create(NULL, 0, len, msg);

  return task;
}

msg_task_t task_message_chain_new(const char *issuer_hostname, const char *mailbox, const char* prev, const char *next, const unsigned int num_pieces)
{
  msg_task_t task = task_message_new(MESSAGE_BUILD_CHAIN, MESSAGE_BUILD_CHAIN_SIZE, issuer_hostname, mailbox);
  message_t msg = MSG_task_get_data(task);
  msg->prev_hostname = xbt_strdup(prev);
  msg->next_hostname = xbt_strdup(next);
  msg->num_pieces = num_pieces;

  return task;
}

msg_task_t task_message_data_new(const char *issuer_hostname, const char *mailbox, const char *block, unsigned int len)
{
  msg_task_t task = task_message_new(MESSAGE_SEND_DATA, MESSAGE_SEND_DATA_HEADER_SIZE + len, issuer_hostname, mailbox);
  message_t msg = MSG_task_get_data(task);
  msg->data_block = block;
  msg->data_length = len;

  return task;
}

void task_message_delete(void *task)
{
  message_t msg = MSG_task_get_data(task);
  xbt_free(msg->issuer_hostname);
  xbt_free(msg->mailbox);
  xbt_free(msg);
  MSG_task_destroy(task);
}
