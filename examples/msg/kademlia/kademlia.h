/* Copyright (c) 2012, 2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _MSG_EXAMPLES_KADEMLIA_H
#define _MSG_EXAMPLES_KADEMLIA_H
#include "node.h"
#include "task.h"

//core kademlia functions
unsigned int join(node_t node, unsigned int id_known);
unsigned int find_node(node_t node, unsigned int id_to_find, unsigned int count_in_stats);
unsigned int ping(node_t node, unsigned int id_to_ping);
void random_lookup(node_t node);

void send_find_node(node_t node, unsigned int id, unsigned int destination);
unsigned int send_find_node_to_best(node_t node, answer_t node_list);

void handle_task(node_t node, msg_task_t task);
void handle_find_node(node_t node, task_data_t data);
void handle_ping(node_t node, task_data_t data);

#endif                          /* _MSG_EXAMPLES_KADEMLIA_H */
