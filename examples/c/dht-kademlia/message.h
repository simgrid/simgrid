/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _DHT_KADEMLIA_MESSAGE
#define _DHT_KADEMLIA_MESSAGE
#include "answer.h"
#include "common.h"
#include "simgrid/mailbox.h"
#include "xbt/sysdep.h"

typedef struct s_kademlia_message {
  unsigned int sender_id;       // Id of the guy who sent the task
  unsigned int destination_id;  // Id we are trying to find, if needed.
  answer_t answer;              // Answer to the request made, if needed.
  sg_mailbox_t answer_to;       // mailbox to send the answer to (if not an answer).
  const char* issuer_host_name; // used for logging
} s_kademlia_message_t;

typedef s_kademlia_message_t* kademlia_message_t;

// Task handling functions
kademlia_message_t task_new_find_node(unsigned int sender_id, unsigned int destination_id, sg_mailbox_t mailbox,
                                      const char* hostname);
kademlia_message_t new_message(unsigned int sender_id, unsigned int destination_id, answer_t answer,
                               sg_mailbox_t mailbox, const char* hostname);
void free_message(void* message);
#endif /* _DHT_KADEMLIA_MESSAGE */
