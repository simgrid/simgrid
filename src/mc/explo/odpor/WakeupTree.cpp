/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "xbt/asserts.h"

#include <algorithm>
#include <exception>
#include <queue>

namespace simgrid::mc::odpor {

aid_t WakeupTreeNode::get_first_actor() const
{
  if (seq_.empty()) {
    throw std::invalid_argument("Attempted to extract the first actor from "
                                "a node in a wakeup tree representing the "
                                "empty execution (likely the root node)");
  }
  return get_sequence().front()->aid_;
}

void WakeupTreeNode::add_child(WakeupTreeNode* node)
{
  this->children_.push_back(node);
  node->parent_ = this;
}

WakeupTreeNode::~WakeupTreeNode()
{
  if (parent_ != nullptr) {
    // TODO: We can probably be more clever here: when
    // we add the child to a node, we could perhaps
    // try instead to keep a reference to position of the
    // child in the list of the parent.
    parent_->children_.remove(this);
  }
}

WakeupTree::WakeupTree() : WakeupTree(std::unique_ptr<WakeupTreeNode>(new WakeupTreeNode({}))) {}
WakeupTree::WakeupTree(std::unique_ptr<WakeupTreeNode> root) : root_(root.get())
{
  this->insert_node(std::move(root));
}

WakeupTree WakeupTree::make_subtree_rooted_at(WakeupTreeNode* root)
{
  if (not root->is_single_process()) {
    throw std::invalid_argument("Selecting subtrees is only defined for single-process nodes");
  }

  const aid_t p = root->get_first_actor();

  // Perform a BFS search to perform a deep copy of the portion
  // of the tree underneath and including `root`. Note that `root`
  // is contained within the context of a *different* wakeup tree;
  // hence, we have to be careful to update each node's children
  // appropriately
  auto subtree = WakeupTree();

  std::list<std::pair<WakeupTreeNode*, WakeupTreeNode*>> frontier{std::make_pair(root, subtree.root_)};
  while (not frontier.empty()) {
    auto [node_in_other_tree, subtree_equivalent] = frontier.front();
    frontier.pop_front();

    // For each child of the node corresponding to that in `subtree`,
    // make clones of each of its children and add them to `frontier`
    // to that their children are added, and so on. Note that the subtree
    // **explicitly** removes the first process from each child
    for (WakeupTreeNode* child_in_other_tree : node_in_other_tree->get_ordered_children()) {
      auto p_w           = child_in_other_tree->get_sequence();
      const auto p_again = p_w.front()->aid_;

      // INVARIANT: A nodes of a wakeup tree form a prefix-closed set;
      // this means that any child node `c` of a node `n` must contain
      // `n.get_sequence()` as a prefix of `c.get_sequence()`
      xbt_assert(p_again == p,
                 "Invariant Violation: The wakeup tree from which a subtree with actor "
                 "`%ld` is being taken is not prefix free! The child node starts with "
                 "`%ld` while the parent is `%ld`! This indicates that there "
                 "is a bug in the insertion logic for the wakeup tree",
                 p, p_again, p);
      p_w.pop_front();

      WakeupTreeNode* child_equivalent = subtree.make_node(p_w);
      frontier.push_back(std::make_pair(child_in_other_tree, child_equivalent));
    }
  }
  return subtree;
}

void WakeupTree::remove_subtree_rooted_at(WakeupTreeNode* root)
{
  if (not contains(root)) {
    throw std::invalid_argument("Attempting to remove a subtree pivoted from a node "
                                "that is not contained in this wakeup tree");
  }

  std::list<WakeupTreeNode*> subtree_contents{root};
  std::list<WakeupTreeNode*> frontier{root};
  while (not frontier.empty()) {
    auto node = frontier.front();
    frontier.pop_front();
    for (const auto& child : node->get_ordered_children()) {
      frontier.push_back(child);
      subtree_contents.push_back(child);
    }
  }

  // After having found each node with BFS, now we can
  // remove them. This prevents the "joys" of iteration during mutation
  for (WakeupTreeNode* node_to_remove : subtree_contents) {
    this->remove_node(node_to_remove);
  }
}

bool WakeupTree::contains(WakeupTreeNode* node) const
{
  return std::find_if(this->nodes_.begin(), this->nodes_.end(), [=](const auto& pair) { return pair.first == node; }) !=
         this->nodes_.end();
}

WakeupTreeNode* WakeupTree::make_node(const PartialExecution& u)
{
  auto node                 = std::unique_ptr<WakeupTreeNode>(new WakeupTreeNode(u));
  auto* node_handle         = node.get();
  this->nodes_[node_handle] = std::move(node);
  return node_handle;
}

void WakeupTree::insert_node(std::unique_ptr<WakeupTreeNode> node)
{
  auto* node_handle         = node.get();
  this->nodes_[node_handle] = std::move(node);
}

void WakeupTree::remove_node(WakeupTreeNode* node)
{
  this->nodes_.erase(node);
}

void WakeupTree::insert(const Execution& E, const PartialExecution& w)
{
  // See section 6.2 of Abdulla. et al.'s 2017 ODPOR paper for details

  // Find the first node `v` in the tree such that
  // `v ~_[E] w` and `v`  is not a leaf node
  for (WakeupTreeNode* node : *this) {
    if (const auto shortest_sequence = E.get_shortest_odpor_sq_subset_insertion(node->get_sequence(), w);
        shortest_sequence.has_value()) {
      // Insert the sequence as a child of `node`, but only
      // if the node is not already a leaf
      if (not node->is_leaf() or node == this->root_) {
        WakeupTreeNode* new_node = this->make_node(shortest_sequence.value());
        node->add_child(new_node);
      }
      // Since we're following the post-order traversal of the tree,
      // the first such node we see is the smallest w.r.t "<"
      return;
    }
  }
  xbt_die("Insertion should always succeed with the root node (which contains no "
          "prior execution). If we've reached this point, this implies either that "
          "the wakeup tree traversal is broken or that computation of the shortest "
          "sequence to insert into the tree is broken");
}

} // namespace simgrid::mc::odpor