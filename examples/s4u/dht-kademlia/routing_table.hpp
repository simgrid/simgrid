/* Copyright (c) 2012-2018. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _KADEMLIA_ROUTING_TABLE_HPP
#define _KADEMLIA_ROUTING_TABLE_HPP
#include "s4u-dht-kademlia.hpp"
#include <deque>

namespace kademlia {

/* Routing table bucket */
class Bucket {
  unsigned int id_; // bucket id
public:
  std::deque<unsigned int> nodes; // Nodes in the bucket.
  unsigned int getId() { return id_; }
  explicit Bucket(unsigned int id) : id_(id) {}
  ~Bucket() = default;
};

/* Node routing table */
class RoutingTable {
  unsigned int id_; // node id of the client's routing table
public:
  Bucket** buckets; // Node bucket list - 160 sized.
  explicit RoutingTable(unsigned int node_id);
  RoutingTable(const RoutingTable&) = delete;
  RoutingTable& operator=(const RoutingTable&) = delete;
  ~RoutingTable();
  void print();
  Bucket* findBucket(unsigned int id);
  bool contains(unsigned int node_id);
};
}

#endif
