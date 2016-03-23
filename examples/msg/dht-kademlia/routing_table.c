/* Copyright (c) 2012-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "routing_table.h"
#include "node.h"
#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_kademlia_routing_table, "Messages specific for this msg example");

/** @brief Initialization of a node routing table.  */
routing_table_t routing_table_init(unsigned int node_id)
{
  unsigned int i;
  routing_table_t table = xbt_new(s_routing_table_t, 1);
  table->buckets = xbt_new(s_bucket_t, identifier_size + 1);
  for (i = 0; i < identifier_size + 1; i++) {
    table->buckets[i].nodes = xbt_dynar_new(sizeof(unsigned int), NULL);
    table->buckets[i].id = i;
  }
  table->id = node_id;
  return table;
}

/** @brief Frees the routing table */
void routing_table_free(routing_table_t table)
{
  unsigned int i;
  //Free the buckets.
  for (i = 0; i <= identifier_size; i++) {
    xbt_dynar_free(&table->buckets[i].nodes);
  }
  xbt_free(table->buckets);
  xbt_free(table);
}

/** Returns if the routing table contains the id. */
unsigned int routing_table_contains(routing_table_t table, unsigned int node_id)
{
  bucket_t bucket = routing_table_find_bucket(table, node_id);
  return bucket_contains(bucket, node_id);
}

/**@brief prints the routing table, to debug stuff. */
void routing_table_print(routing_table_t table)
{
  unsigned int i, j, value;
  XBT_INFO("Routing table of %08x:", table->id);

  for (i = 0; i <= identifier_size; i++) {
    if (!xbt_dynar_is_empty(table->buckets[i].nodes)) {
      XBT_INFO("Bucket number %d: ", i);
      xbt_dynar_foreach(table->buckets[i].nodes, j, value) {
        XBT_INFO("Element %d: %08x", j, value);
      }
    }
  }
}

/** @brief Finds an identifier in a bucket and returns its position or returns -1 if it doesn't exists
  * @param bucket the bucket in which we try to find our identifier
  * @param id the id
  */
unsigned int bucket_find_id(bucket_t bucket, unsigned int id)
{
  unsigned int i, current_id;
  xbt_dynar_foreach(bucket->nodes, i, current_id){
    if (id == current_id){
      return i;
    }
  }
  return -1;
}

/** Returns if the bucket contains an identifier.  */
unsigned int bucket_contains(bucket_t bucket, unsigned int id)
{
  return xbt_dynar_member(bucket->nodes, &id);
}

/** @brief Finds the corresponding bucket in a routing table for a given identifier
  * @param table the routing table
  * @param id the identifier
  * @return the bucket in which the the identifier would be.
  */
bucket_t routing_table_find_bucket(routing_table_t table, unsigned int id)
{
  unsigned int xor_number = table->id ^ id;
  unsigned int prefix = get_node_prefix(xor_number, identifier_size);
  xbt_assert(prefix >= 0 && prefix <= identifier_size, "Tried to return a  bucket that doesn't exist.");
  bucket_t bucket = &table->buckets[prefix];
  return bucket;
}
