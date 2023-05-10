/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_WAKEUP_TREE_HPP
#define SIMGRID_MC_ODPOR_WAKEUP_TREE_HPP

#include "src/mc/explo/odpor/WakeupTreeIterator.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"

#include <memory>
#include <unordered_map>

namespace simgrid::mc::odpor {

class WakeupTreeNode {
private:
  explicit WakeupTreeNode(const PartialExecution& u) : seq_(u) {}
  explicit WakeupTreeNode(PartialExecution&& u) : seq_(std::move(u)) {}

  /** An ordered list of children of for this node in the tree */
  std::list<WakeupTreeNode*> children_;

  /** @brief The contents of the node */
  PartialExecution seq_;

  /** Allows the owning tree to insert directly into the child */
  friend WakeupTree;

public:
  WakeupTreeNode(const WakeupTreeNode&)            = delete;
  WakeupTreeNode(WakeupTreeNode&&)                 = default;
  WakeupTreeNode& operator=(const WakeupTreeNode&) = delete;
  WakeupTreeNode& operator=(WakeupTreeNode&&)      = default;

  const auto begin() const { return this->children_.begin(); }
  const auto end() const { return this->children_.end(); }
  const auto rbegin() const { return this->children_.rbegin(); }
  const auto rend() const { return this->children_.rend(); }

  const PartialExecution& get_sequence() const { return seq_; }
  const std::list<WakeupTreeNode*>& get_ordered_children() const { return children_; }
  bool is_leaf() const { return children_.empty(); }
  bool is_single_process() const { return children_.size() == static_cast<size_t>(1); }

  /** Insert a node `node` as a new child of this node */
  void add_child(WakeupTreeNode* node) { this->children_.push_back(node); }
};

class WakeupTree {
private:
  /** @brief The root node of the tree */
  WakeupTreeNode* const root_;

  /**
   * @brief All of the nodes that are currently are a part of the tree
   *
   * @invariant Each node event maps itself to the owner of that node,
   * i.e. the unique pointer that manages the data at the address. The tree owns all
   * of the addresses that are referenced by the nodes WakeupTreeNode and Configuration.
   * ODPOR guarantees that nodes are persisted as long as needed.
   */
  std::unordered_map<WakeupTreeNode*, std::unique_ptr<WakeupTreeNode>> nodes_;

  /**
   * @brief Transfers the lifetime of node `node` to the tree
   */
  void insert_node(std::unique_ptr<WakeupTreeNode> node);

  /**
   * @brief Creates a new, disconnected node in this tree
   */
  WakeupTreeNode* make_node(const PartialExecution& u);

  /* Allow the iterator to access the contents of the tree */
  friend WakeupTreeIterator;

public:
  WakeupTree();
  explicit WakeupTree(std::unique_ptr<WakeupTreeNode> root);
  static WakeupTree new_subtree_rooted_at(WakeupTreeNode* root);

  auto begin() const { return WakeupTreeIterator(*this); }
  auto end() const { return WakeupTreeIterator(); }

  /**
   * @brief Inserts an sequence `seq` of processes into the tree
   * such that that this tree is a wakeup tree relative to the
   * given execution
   */
  void insert(const Execution& E, const PartialExecution& seq);
};

} // namespace simgrid::mc::odpor
#endif
