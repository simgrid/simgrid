/* Copyright (c) 2012-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _KADEMLIA_ANSWER_HPP_
#define _KADEMLIA_ANSWER_HPP_

#include "node.hpp"
#include "routing_table.hpp"
#include <set>

namespace kademlia {
bool sortbydistance(const std::pair<unsigned int, unsigned int>& a, const std::pair<unsigned int, unsigned int>& b);

/* Node query answer. contains the elements closest to the id given. */
class Answer {
  unsigned int destination_id_;
  unsigned int size_ = 0;

public:
  std::vector<std::pair<unsigned int, unsigned int>> nodes;
  explicit Answer(unsigned int destination_id) : destination_id_(destination_id) {}
  virtual ~Answer() = default;
  unsigned int getDestinationId() const { return destination_id_; }
  unsigned int getSize() { return size_; }
  void print();
  unsigned int merge(const Answer* a);
  void trim();
  bool destinationFound();
  void addBucket(const kademlia::Bucket* bucket);
};
}

#endif
