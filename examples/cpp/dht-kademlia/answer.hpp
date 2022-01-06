/* Copyright (c) 2012-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _KADEMLIA_ANSWER_HPP_
#define _KADEMLIA_ANSWER_HPP_

#include "node.hpp"
#include "routing_table.hpp"
#include <set>

namespace kademlia {
/* Node query answer. contains the elements closest to the id given. */
class Answer {
  unsigned int destination_id_;
  std::vector<std::pair<unsigned int, unsigned int>> nodes_;

public:
  explicit Answer(unsigned int destination_id) : destination_id_(destination_id) {}
  unsigned int getDestinationId() const { return destination_id_; }
  size_t getSize() const { return nodes_.size(); }
  const std::vector<std::pair<unsigned int, unsigned int>>& getNodes() const { return nodes_; }
  void print() const;
  unsigned int merge(const Answer* a);
  void trim();
  bool destinationFound() const;
  void addBucket(const kademlia::Bucket* bucket);
};
} // namespace kademlia

#endif
