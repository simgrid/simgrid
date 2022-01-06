/* Copyright (c) 2012-2022. The SimGrid Team.
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
  buckets_.reserve(IDENTIFIER_SIZE + 1);
  for (unsigned int i = 0; i < IDENTIFIER_SIZE + 1; i++)
    buckets_.emplace_back(i);
}

void RoutingTable::print() const
{
  XBT_INFO("Routing table of %08x:", id_);

  for (unsigned int i = 0; i <= IDENTIFIER_SIZE; i++) {
    if (not buckets_[i].nodes_.empty()) {
      XBT_INFO("Bucket number %u: ", i);
      int j = 0;
      for (auto value : buckets_[i].nodes_) {
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
  unsigned int prefix     = get_node_prefix(xor_number, IDENTIFIER_SIZE);
  xbt_assert(prefix <= IDENTIFIER_SIZE, "Tried to return a  bucket that doesn't exist.");
  return &buckets_[prefix];
}

/** Returns if the routing table contains the id. */
bool RoutingTable::contains(unsigned int node_id)
{
  const Bucket* bucket = findBucket(node_id);
  return std::find(bucket->nodes_.begin(), bucket->nodes_.end(), node_id) != bucket->nodes_.end();
}
} // namespace kademlia
