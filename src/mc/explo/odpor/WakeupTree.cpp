/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/odpor/Execution.hpp"

#include <algorithm>

namespace simgrid::mc::odpor {

WakeupTree::WakeupTree() : root(nullptr) {}

WakeupTreeNode* WakeupTree::make_node(const ProcessSequence& u)
{
  auto node                = std::unique_ptr<WakeupTreeNode>(new WakeupTreeNode(u));
  auto* node_handle        = node.get();
  this->nodes_[node.get()] = std::move(node);
  return node_handle;
}

void WakeupTree::insert(const Execution& E, const ExecutionSequence& w)
{
  // See section 6.2 of Abdulla. et al.'s 2017 ODPOR paper for details

  // Find the first node `v` in the tree such that
  // `v ~_[E] w` and `v`  is not a leaf node
  for (WakeupTreeNode* node : *this) {
    if (const auto shortest_sequence = E.get_shortest_odpor_sq_subset_insertion(node->get_sequence(), w);
        shortest_sequence.has_value()) {
      // Insert the sequence as a child of `node`, but only
      // if the node is not already a leaf
      if (not node->is_leaf()) {
        WakeupTreeNode* new_node = this->make_node(shortest_sequence.value());
        node->add_child(new_node);
      }
      // Since we're following the post-order traversal of the tree,
      // the first such node we see is the smallest w.r.t "<"
      return;
    }
  }
}

} // namespace simgrid::mc::odpor