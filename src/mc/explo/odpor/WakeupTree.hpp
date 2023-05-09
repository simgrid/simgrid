/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_WAKEUP_TREE_HPP
#define SIMGRID_MC_ODPOR_WAKEUP_TREE_HPP

#include "src/mc/explo/odpor/odpor_forward.hpp"

#include <memory>
#include <unordered_map>

namespace simgrid::mc::odpor {

class WakeupTreeNode {
private:
  /** An ordered list of children of for this node in the tree */
  std::list<const WakeupTreeNode*> children_;

  /** @brief The contents of the node */
  ProcessSequence seq_;

public:
  const auto begin() const { return this->children_.begin(); }
  const auto end() const { return this->children_.end(); }
  const auto rbegin() const { return this->children_.rbegin(); }
  const auto rend() const { return this->children_.rend(); }

  const ProcessSequence& get_sequence() const { return seq_; }
  const std::list<const WakeupTreeNode*>& get_ordered_children() const { return children_; }

  bool is_leaf() const { return children_.empty(); }
};

class WakeupTree {
private:
  /** @brief The root node of the tree */
  const WakeupTreeNode* const root;

  /**
   * @brief All of the nodes that are currently are a part of the tree
   *
   * @invariant Each node event maps itself to the owner of that node,
   * i.e. the unique pointer that manages the data at the address. The tree owns all
   * of the addresses that are referenced by the nodes WakeupTreeNode and Configuration.
   * ODPOR guarantees that nodes are persisted as long as needed.
   */
  std::unordered_map<const WakeupTreeNode*, std::unique_ptr<WakeupTreeNode>> nodes_;

  /* Allow the iterator to access the contents of the tree */
  friend WakeupTreeIterator;

public:
  WakeupTree();

  /**
   * @brief Inserts an sequence `seq` of processes into the tree
   * such that that this tree is a wakeup tree relative to the
   * given execution
   */
  void insert(const Execution&, const ExecutionSequence& seq);
};

} // namespace simgrid::mc::odpor
#endif
