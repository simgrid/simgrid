/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "answer.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(kademlia_node);

namespace kademlia {

/** @brief Prints a answer_t, for debugging purposes */
void Answer::print() const
{
  XBT_INFO("Searching %08x, size %zu", destination_id_, nodes_.size());
  unsigned int i = 0;
  for (auto const& contact : nodes_)
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
  for (auto const& contact : source->nodes_) {
    if (std::find(nodes_.begin(), nodes_.end(), contact) == nodes_.end()) {
      nodes_.push_back(contact);
      nb_added++;
    }
  }
  trim();
  return nb_added;
}

/** @brief Trims an Answer, in order for it to have a size of less or equal to "bucket_size" */
void Answer::trim()
{
  // sort by distance
  std::sort(nodes_.begin(), nodes_.end(),
            [](const std::pair<unsigned int, unsigned int>& a, const std::pair<unsigned int, unsigned int>& b) {
              return (a.second < b.second);
            });
  if (nodes_.size() > BUCKET_SIZE)
    nodes_.resize(BUCKET_SIZE);
}

/** @brief Returns if the destination we are trying to find is found
  * @return if the destination is found.
  */
bool Answer::destinationFound() const
{
  return not nodes_.empty() && nodes_.begin()->second == 0;
}

/** @brief Adds the content of a bucket unsigned into a answer object.
  * @param bucket the bucket we have to had unsigned into
  */
void Answer::addBucket(const Bucket* bucket)
{
  xbt_assert((bucket != nullptr), "Provided a NULL bucket");

  for (auto const& id : bucket->nodes_) {
    unsigned int distance = id ^ destination_id_;
    nodes_.push_back(std::pair<unsigned int, unsigned int>(id, distance));
  }
}
} // namespace kademlia
