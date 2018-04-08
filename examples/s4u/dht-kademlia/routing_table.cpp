/* Copyright (c) 2012-2018. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "routing_table.hpp"
#include "node.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(kademlia_routing_table, "Messages specific for this example");

namespace kademlia {

/** @brief Initialization of a node routing table.  */
RoutingTable::RoutingTable(unsigned int node_id) : id_(node_id)
{
  buckets = new Bucket*[identifier_size + 1];
  for (unsigned int i = 0; i < identifier_size + 1; i++)
    buckets[i]        = new Bucket(i);
}

RoutingTable::~RoutingTable()
{
  // Free the buckets.
  for (unsigned int i = 0; i <= identifier_size; i++) {
    delete buckets[i];
  }
  delete[] buckets;
}

void RoutingTable::print()
{
  XBT_INFO("Routing table of %08x:", id_);

  for (unsigned int i = 0; i <= identifier_size; i++) {
    if (not buckets[i]->nodes.empty()) {
      XBT_INFO("Bucket number %u: ", i);
      int j = 0;
      for (auto value : buckets[i]->nodes) {
        XBT_INFO("Element %d: %08x", j, value);
        j++;
      }
    }
  }
}

/** @brief Finds the corresponding bucket in a routing table for a given identifier
  * @param id the identifier
  * @return the bucket in which the the identifier would be.
  */
Bucket* RoutingTable::findBucket(unsigned int id)
{
  unsigned int xor_number = id_ ^ id;
  unsigned int prefix     = get_node_prefix(xor_number, identifier_size);
  xbt_assert(prefix <= identifier_size, "Tried to return a  bucket that doesn't exist.");
  return buckets[prefix];
}

/** Returns if the routing table contains the id. */
bool RoutingTable::contains(unsigned int node_id)
{
  Bucket* bucket = findBucket(node_id);
  return std::find(bucket->nodes.begin(), bucket->nodes.end(), node_id) != bucket->nodes.end();
}
}
