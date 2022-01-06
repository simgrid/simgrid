/* Copyright (c) 2012-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _KADEMLIA_ROUTING_TABLE_HPP
#define _KADEMLIA_ROUTING_TABLE_HPP
#include "s4u-dht-kademlia.hpp"
#include <deque>
#include <vector>

namespace kademlia {

/* Routing table bucket */
class Bucket {
public:
  const unsigned int id_;          // bucket id
  std::deque<unsigned int> nodes_; // Nodes in the bucket.
  unsigned int getId() const { return id_; }
  explicit Bucket(unsigned int id) : id_(id) {}
  // Use rule-of-three, and implicitely disable the move constructor which cannot be 'noexcept' (as required by C++ Core
  // Guidelines), due to the std::deque member.
  Bucket(const Bucket&) = default;
  ~Bucket()             = default;
};

/* Node routing table */
class RoutingTable {
  unsigned int id_; // node id of the client's routing table
  std::vector<Bucket> buckets_; // Node bucket list
public:
  explicit RoutingTable(unsigned int node_id);
  RoutingTable(const RoutingTable&) = delete;
  RoutingTable& operator=(const RoutingTable&) = delete;
  void print() const;
  Bucket* findBucket(unsigned int id);
  const Bucket& getBucketAt(unsigned int pos) const { return buckets_[pos]; }
  bool contains(unsigned int node_id);
};
} // namespace kademlia

#endif
