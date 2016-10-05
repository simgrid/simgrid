/* Copyright (c) 2012, 2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _KADEMLIA_EXAMPLES_ANSWER_H_
#define _KADEMLIA_EXAMPLES_ANSWER_H_
#include <xbt/dynar.h>
#include "routing_table.h"

/* Node query anwser. contains the elements closest to the id given. */
typedef struct s_node_answer {
  unsigned int destination_id;
  xbt_dynar_t nodes;            //Dynar of node_contact_t
  unsigned int size;
} s_answer_t, *answer_t;

#include "node.h"

answer_t answer_init(unsigned int destination_id);
void answer_free(answer_t answer);
void answer_print(answer_t answer);
unsigned int answer_merge(answer_t destination, answer_t source);
void answer_sort(answer_t answer);
void answer_trim(answer_t answer);
void answer_add_bucket(bucket_t bucket, answer_t answer);
unsigned int answer_contains(answer_t answer, unsigned int id);
unsigned int answer_destination_found(answer_t answer);

#endif                          /* _KADEMLIA_EXAMPLES_ANSWER_H_ */
