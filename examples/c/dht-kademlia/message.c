/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "message.h"
#include "answer.h"

kademlia_message_t new_message(unsigned int sender_id, unsigned int destination_id, answer_t answer,
                               sg_mailbox_t mailbox, const char* hostname)
{
  kademlia_message_t msg = xbt_new(s_kademlia_message_t, 1);

  msg->sender_id        = sender_id;
  msg->destination_id   = destination_id;
  msg->answer           = answer;
  msg->answer_to        = mailbox;
  msg->issuer_host_name = hostname;

  return msg;
}

void free_message(void* message)
{
  const kademlia_message_t msg = (kademlia_message_t)message;
  if (msg)
    answer_free(msg->answer);
  free(msg);
}
