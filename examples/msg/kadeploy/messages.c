#include "messages.h"

msg_task_t task_message_new(e_message_type type, const char *issuer_hostname, const char *mailbox)
{
  message_t msg = xbt_new(s_message_t, 1);
  msg->type = type;
  msg->issuer_hostname = issuer_hostname;
  msg->mailbox = mailbox;
  msg_task_t task = MSG_task_create(NULL, 0, MESSAGE_SIZE, msg); 

  return task;
}

msg_task_t task_message_chain_new(const char *issuer_hostname, const char *mailbox, const char* prev, const char *next)
{
  msg_task_t task = task_message_new(MESSAGE_BUILD_CHAIN, issuer_hostname, mailbox);
  message_t msg = MSG_task_get_data(task);
  msg->prev_hostname = prev;
  msg->next_hostname = next;

  return task;
}

msg_task_t task_message_data_new(const char *issuer_hostname, const char *mailbox, const char *block, unsigned int len)
{
  msg_task_t task = task_message_new(MESSAGE_SEND_DATA, issuer_hostname, mailbox);
  //if (strcmp(mailbox, "host4") == 0) 
  //MSG_task_set_category(task, mailbox);
  message_t msg = MSG_task_get_data(task);
  msg->data_block = block;
  msg->data_length = len;

  return task;
}

msg_task_t task_message_end_data_new(const char *issuer_hostname, const char *mailbox)
{
  return task_message_new(MESSAGE_END_DATA, issuer_hostname, mailbox);
}

void task_message_delete(void *task)
{
  message_t msg = MSG_task_get_data(task);
  xbt_free(msg);
  MSG_task_destroy(task);
}
