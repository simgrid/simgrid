/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _DHT_KADEMLIA_ROUTING_H
#define _DHT_KADEMLIA_ROUTING_H
#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "simgrid/mailbox.h"
#include "xbt/dynar.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#include "answer.h"
#include "common.h"
#include "message.h"
#include "routing_table.h"

/* Information about a foreign node */
typedef struct s_node_contact {
  unsigned int id;       // The node identifier
  unsigned int distance; // The distance from the node
} s_node_contact_t;

typedef s_node_contact_t* node_contact_t;
typedef const s_node_contact_t* const_node_contact_t;

/* Node data */
typedef struct s_node {
  unsigned int id;        // node id - 160 bits
  routing_table_t table;  // node routing table
  sg_comm_t receive_comm; // current receiving communication.
  void* received_msg;     // current task being received

  sg_mailbox_t mailbox;           // node mailbox
  unsigned int find_node_success; // Number of find_node which have succeeded.
  unsigned int find_node_failed;  // Number of find_node which have failed.
} s_node_t;

typedef s_node_t* node_t;
typedef const s_node_t* const_node_t;

// node functions
node_t node_init(unsigned int id);
void node_free(node_t node);
void routing_table_update(const_node_t node, unsigned int id);
answer_t find_closest(const_node_t node, unsigned int destination_id);

// identifier functions
unsigned int get_id_in_prefix(unsigned int id, unsigned int prefix);
unsigned int get_node_prefix(unsigned int id, unsigned int nb_bits);
sg_mailbox_t get_node_mailbox(unsigned int id);

// node contact functions
node_contact_t node_contact_new(unsigned int id, unsigned int distance);
node_contact_t node_contact_copy(const_node_contact_t node_contact);
void node_contact_free(node_contact_t contact);

unsigned int join(node_t node, unsigned int id_known);
unsigned int find_node(node_t node, unsigned int id_to_find, unsigned int count_in_stats);
void random_lookup(node_t node);

void send_find_node(const_node_t node, unsigned int id, unsigned int destination);
unsigned int send_find_node_to_best(node_t node, const_answer_t node_list);

void handle_find_node(const_node_t node, const_kademlia_message_t data);

#endif
