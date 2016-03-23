/* Copyright (c) 2012, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _MSG_KADEMLIA_EXAMPLES_ROUTING_TABLE
#define _MSG_KADEMLIA_EXAMPLES_ROUTING_TABLE
#include "common.h"
#include <xbt/dynar.h>

/* Routing table bucket */
typedef struct s_bucket {
  xbt_dynar_t nodes;            //Nodes in the bucket.
  unsigned int id;              //bucket id
} s_bucket_t, *bucket_t;

/* Node routing table */
typedef struct s_routing_table {
  unsigned int id;              //node id of the client's routing table
  s_bucket_t *buckets;          //Node bucket list - 160 sized.
} s_routing_table_t, *routing_table_t;

// bucket functions
unsigned int bucket_find_id(bucket_t bucket, unsigned int id);
unsigned int bucket_contains(bucket_t bucket, unsigned int id);

// routing table functions
routing_table_t routing_table_init(unsigned int node_id);
void routing_table_free(routing_table_t table);
unsigned int routing_table_contains(routing_table_t table, unsigned int node_id);
void routing_table_print(routing_table_t table);
bucket_t routing_table_find_bucket(routing_table_t table, unsigned int id);

#endif                          /* _MSG_KADEMLIA_EXAMPLES_ROUTING_TABLE */
