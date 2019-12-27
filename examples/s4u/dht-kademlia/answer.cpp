/* Copyright (c) 2012-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "answer.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(kademlia_node);

namespace kademlia {

bool sortbydistance(const std::pair<unsigned int, unsigned int>& a, const std::pair<unsigned int, unsigned int>& b)
{
  return (a.second < b.second);
}

/** @brief Prints a answer_t, for debugging purposes */
void Answer::print()
{
  XBT_INFO("Searching %08x, size %u", destination_id_, size_);
  unsigned int i = 0;
  for (auto contact : nodes)
    XBT_INFO("Node %08x: %08x is at distance %u", i++, contact.first, contact.second);
}

/** @brief Merge two answers together, only keeping the best nodes
  * @param source the source of the nodes to add
  */
unsigned int Answer::merge(const Answer* source)
{
  if (this == source)
    return 0;

  unsigned int nb_added = 0;
  for (auto contact : source->nodes) {
    if (std::find(nodes.begin(), nodes.end(), contact) == nodes.end()) {
      nodes.push_back(contact);
      size_++;
      nb_added++;
    }
  }
  std::sort(nodes.begin(), nodes.end(), sortbydistance);
  trim();
  return nb_added;
}

/** @brief Trims an Answer, in order for it to have a size of less or equal to "bucket_size" */
void Answer::trim()
{
  while (size_ > BUCKET_SIZE) {
    nodes.pop_back();
    size_--;
  }
  xbt_assert(nodes.size() == size_, "Wrong size for the answer");
}

/** @brief Returns if the destination we are trying to find is found
  * @return if the destination is found.
  */
bool Answer::destinationFound()
{
  if (nodes.empty())
    return 0;

  return (*nodes.begin()).second == 0;
}

/** @brief Adds the content of a bucket unsigned into a answer object.
  * @param bucket the bucket we have to had unsigned into
  */
void Answer::addBucket(const Bucket* bucket)
{
  xbt_assert((bucket != nullptr), "Provided a NULL bucket");

  for (auto id : bucket->nodes) {
    unsigned int distance = id ^ destination_id_;
    nodes.push_back(std::pair<unsigned int, unsigned int>(id, distance));
    size_++;
  }
}
}
