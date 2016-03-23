/* Copyright (c) 2012, 2014-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _MSG_EXAMPLES_ROUTING_H
#define _MSG_EXAMPLES_ROUTING_H
#include "xbt/dynar.h"
#include "simgrid/msg.h"

#include "common.h"
#include "answer.h"
#include "routing_table.h"

/* Information about a foreign node */
typedef struct s_node_contact {
  unsigned int id;              //The node identifier
  unsigned int distance;        //The distance from the node
} s_node_contact_t, *node_contact_t;

/* Node data */
typedef struct s_node {
  unsigned int id;              //node id - 160 bits
  routing_table_t table;        //node routing table
  msg_comm_t receive_comm;      //current receiving communication.
  msg_task_t task_received;     //current task being received

  char mailbox[MAILBOX_NAME_SIZE+1];      //node mailbox
  unsigned int find_node_success;       //Number of find_node which have succeeded.
  unsigned int find_node_failed;        //Number of find_node which have failed.

} s_node_t, *node_t;

// node functions
node_t node_init(unsigned int id);
void node_free(node_t node);
void node_routing_table_update(node_t node, unsigned int id);
answer_t node_find_closest(node_t node, unsigned int destination_id);

// identifier functions
unsigned int get_id_in_prefix(unsigned int id, unsigned int prefix);
unsigned int get_node_prefix(unsigned int id, unsigned int nb_bits);
void get_node_mailbox(unsigned int id, char *mailbox);

// node contact functions
node_contact_t node_contact_new(unsigned int id, unsigned int distance);
node_contact_t node_contact_copy(node_contact_t node_contact);
void node_contact_free(node_contact_t contact);
#endif                          /* _MSG_EXAMPLES_ROUTING_H */
