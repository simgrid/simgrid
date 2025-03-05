/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "simgrid/forward.h"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/mc_config.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <algorithm>
#include <memory>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_wut, mc, "Logging specific to ODPOR WakeupTrees");

namespace simgrid::mc::odpor {

void WakeupTreeNode::add_child(std::unique_ptr<WakeupTreeNode> node)
{
  xbt_assert(node != nullptr, "Who gave me a nullptr??");
  node->parent_   = this;
  node->sequence_ = this->sequence_;
  node->sequence_.emplace_back(node->action_);
  this->children_.push_back(std::move(node));
}

std::string WakeupTreeNode::string_of_whole_tree(const std::string& prefix, bool is_first, bool is_last) const
{

  std::string final_string = action_ == nullptr
                                 ? "<>\n"
                                 : prefix + (is_last ? "└──" : "├──") + "Actor " + std::to_string(action_->aid_) +
                                       ": " + action_->to_string(true) + "\n";
  bool is_next_first       = true;
  for (auto node_it = children_.begin(); node_it != children_.end(); node_it++) {
    auto node                    = node_it;
    const std::string new_prefix = prefix + (is_last ? "    " : "│   ");
    final_string += node->get()->string_of_whole_tree(new_prefix, is_next_first, std::next(node_it) == children_.end());
    is_next_first = false;
  }
  return final_string;
}

const PartialExecution& WakeupTreeNode::get_sequence() const
{
  return sequence_;
}

WakeupTree::WakeupTree() : WakeupTree(std::make_unique<WakeupTreeNode>()) {}
WakeupTree::WakeupTree(std::unique_ptr<WakeupTreeNode> root) : root_(std::move(root)) {}

std::vector<std::string> WakeupTree::get_single_process_texts() const
{
  std::vector<std::string> trace;
  for (const auto& child : root_->children_) {
    const auto t = child->get_action();
    auto message = xbt::string_printf("Actor %ld: %s", t->aid_, t->to_string(true).c_str());
    trace.emplace_back(std::move(message));
  }
  return trace;
}

std::optional<aid_t> WakeupTree::get_min_single_process_actor() const
{
  if (const auto node = get_min_single_process_node(); node.has_value()) {
    return node.value()->get_actor();
  }
  return std::nullopt;
}

std::optional<WakeupTreeNode*> WakeupTree::get_min_single_process_node() const
{
  if (empty()) {
    return std::nullopt;
  }
  // INVARIANT: The induced post-order relation always places children
  // in order before the parent. The list of children maintained by
  // each node represents that ordering, and the first child of
  // the root is by definition the smallest of the single-process nodes
  xbt_assert(not this->root_->children_.empty(), "What the");
  return this->root_->children_.front().get();
}

std::string WakeupTree::string_of_whole_tree() const
{
  return "\n" + root_->string_of_whole_tree("", false, true);
}

WakeupTree WakeupTree::get_first_subtree()
{
  auto subtree = WakeupTree();
  if (empty())
    return subtree;

  auto node_it = this->root_->children_.begin();

  subtree.root_ = std::move(*node_it);
  root_->children_.erase(node_it);

  subtree.root_->parent_   = nullptr;
  subtree.root_->action_   = nullptr;
  subtree.root_->sequence_ = odpor::PartialExecution();

  return subtree;
}

void WakeupTree::remove_subtree_at_aid(aid_t proc)
{
  auto child = std::find_if(root_->children_.begin(), root_->children_.end(),
                            [&](const auto& node) { return node->get_actor() == proc; });
  if (child != root_->children_.end())
    root_->children_.erase(child);
}

bool WakeupTree::contains(const WakeupTreeNode* node) const
{
  return std::find_if(this->root_->children_.begin(), this->root_->children_.end(),
                      [=](const auto& pair) { return pair.get() == node; }) != this->root_->children_.end();
}

void WakeupTree::insert_at_root(std::shared_ptr<Transition> u)
{
  std::unique_ptr<WakeupTreeNode> new_node = std::make_unique<WakeupTreeNode>(std::move(u));
  this->root_->add_child(std::move(new_node));
}

InsertionResult WakeupTreeNode::recursive_insert(WakeupTree& father, PartialExecution& w)
{
  // If we reached a leaf, then there is nothing to insert. The exploration from this leaf
  // will eventually take care of this execution if needed
  if (this->is_leaf() and not this->is_root())
    return InsertionResult::leaf;

  for (auto& node : this->children_) {
    const auto& next_E_p = node->action_;
    // Is `p in `I_[E](w)`?
    if (const aid_t p = next_E_p->aid_; Execution::is_initial_after_execution_of(w, p)) {
      // Remove `p` from w and continue with this node

      // INVARIANT: If `p` occurs in `w`, it had better refer to the same
      // transition referenced by `v`. Unfortunately, we have two
      // sources of truth here which can be manipulated at the same
      // time as arguments to the function. If ODPOR works correctly,
      // they should always refer to the same value; but as a sanity check,
      // we have an assert that tests that at least the types are the same.
      const auto action_by_p_in_w =
          std::find_if(w.begin(), w.end(), [=](const auto& action) { return action->aid_ == p; });
      xbt_assert(action_by_p_in_w != w.end(), "Invariant violated: actor `p` "
                                              "is claimed to be an initial after `w` but is "
                                              "not actually contained in `w`. This indicates that there "
                                              "is a bug computing initials");
      if (_sg_mc_debug) {
        const auto& w_action = *action_by_p_in_w;
        xbt_assert(w_action->type_ == next_E_p->type_,
                   "Invariant violated: `v` claims that actor `%ld` executes '%s' while "
                   "`w` claims that it executes '%s'. These two partial executions both "
                   "refer to `next_[E](p)`, which should be the same",
                   p, next_E_p->to_string(false).c_str(), w_action->to_string(false).c_str());
      }
      w.erase(action_by_p_in_w);
      return node->recursive_insert(father, w);
    }
    // Is `E ⊢ p ◇ w`?
    else if (Execution::is_independent_with_execution_of(w, next_E_p)) {
      // Nothing to remove, we simply move on

      // INVARIANT: Note that it is impossible for `p` to be
      // excluded from the set `I_[E](w)` BUT ALSO be contained in
      // `w` itself if `E ⊢ p ◇ w` (intuitively, the fact that `E ⊢ p ◇ w`
      // means that are able to move `p` anywhere in `w` IF it occurred, so
      // if it really does occur we know it must then be an initial).
      // We assert this is the case here
      const auto action_by_p_in_w =
          std::find_if(w.begin(), w.end(), [=](const auto& action) { return action->aid_ == p; });
      xbt_assert(action_by_p_in_w == w.end(),
                 "Invariant violated: We claimed that actor `%ld` is not an initial "
                 "after `w`, yet it's independent with all actions of `w` AND occurs in `w`."
                 "This indicates that there is a bug computing initials",
                 p);
      return node->recursive_insert(father, w);
    }
  }
  // We can't insert w anymore from there. Let's insert the remaining and tell the world about that
  father.insert_sequence_after(this, w);
  return this->is_root() ? InsertionResult::root : InsertionResult::interior_node;
}

InsertionResult WakeupTree::insert(const PartialExecution& w)
{
  auto w_copy = w;
  return this->root_->recursive_insert(*this, w_copy);
}

WakeupTreeNode* WakeupTreeNode::recursive_insert_and_get_inserted_seq(WakeupTree& father, PartialExecution& w)
{
  // If we reached a leaf, then there is nothing to insert. The exploration from this leaf
  // will eventually take care of this execution if needed
  if (this->is_leaf())
    return this;

  for (auto& node : this->children_) {
    const auto& next_E_p = node->action_;
    // Is `p in `I_[E](w)`?
    if (const aid_t p = next_E_p->aid_; Execution::is_initial_after_execution_of(w, p)) {
      // Remove `p` from w and continue with this node

      // INVARIANT: If `p` occurs in `w`, it had better refer to the same
      // transition referenced by `v`. Unfortunately, we have two
      // sources of truth here which can be manipulated at the same
      // time as arguments to the function. If ODPOR works correctly,
      // they should always refer to the same value; but as a sanity check,
      // we have an assert that tests that at least the types are the same.
      const auto action_by_p_in_w =
          std::find_if(w.begin(), w.end(), [=](const auto& action) { return action->aid_ == p; });
      xbt_assert(action_by_p_in_w != w.end(), "Invariant violated: actor `p` "
                                              "is claimed to be an initial after `w` but is "
                                              "not actually contained in `w`. This indicates that there "
                                              "is a bug computing initials");
      const auto& w_action = *action_by_p_in_w;
      xbt_assert(w_action->type_ == next_E_p->type_,
                 "Invariant violated: `v` claims that actor `%ld` executes '%s' while "
                 "`w` claims that it executes '%s'. These two partial executions both "
                 "refer to `next_[E](p)`, which should be the same",
                 p, next_E_p->to_string(false).c_str(), w_action->to_string(false).c_str());
      w.erase(action_by_p_in_w);
      return node->recursive_insert_and_get_inserted_seq(father, w);
    }
    // Is `E ⊢ p ◇ w`?
    else if (Execution::is_independent_with_execution_of(w, next_E_p)) {
      // Nothing to remove, we simply move on

      // INVARIANT: Note that it is impossible for `p` to be
      // excluded from the set `I_[E](w)` BUT ALSO be contained in
      // `w` itself if `E ⊢ p ◇ w` (intuitively, the fact that `E ⊢ p ◇ w`
      // means that are able to move `p` anywhere in `w` IF it occurred, so
      // if it really does occur we know it must then be an initial).
      // We assert this is the case here
      const auto action_by_p_in_w =
          std::find_if(w.begin(), w.end(), [=](const auto& action) { return action->aid_ == p; });
      xbt_assert(action_by_p_in_w == w.end(),
                 "Invariant violated: We claimed that actor `%ld` is not an initial "
                 "after `w`, yet it's independent with all actions of `w` AND occurs in `w`."
                 "This indicates that there is a bug computing initials",
                 p);
      return node->recursive_insert_and_get_inserted_seq(father, w);
    }
  }
  // We can't insert w anymore from there. Let's insert the remaining and tell the world about that
  return father.insert_sequence_after(this, w);
}

const PartialExecution WakeupTree::insert_and_get_inserted_seq(const PartialExecution& w)
{
  auto w_copy                   = w;
  WakeupTreeNode* inserted_node = this->root_->recursive_insert_and_get_inserted_seq(*this, w_copy);
  return inserted_node->get_sequence();
}

WakeupTreeNode* WakeupTree::insert_sequence_after(WakeupTreeNode* node, const PartialExecution& w)
{
  WakeupTreeNode* cur_node = node;
  for (const auto& w_i : w) {
    std::unique_ptr<WakeupTreeNode> new_node = std::make_unique<WakeupTreeNode>(std::move(w_i));
    auto tmp                                 = new_node.get();
    cur_node->add_child(std::move(new_node));
    cur_node = tmp;
  }
  return cur_node;
}

WakeupTreeNode* WakeupTree::get_node_after_actor(aid_t aid) const
{
  for (auto const& node : root_->children_)
    if (node->get_actor() == aid)
      return node.get();

  return nullptr;
}

bool WakeupTreeNode::have_same_content(const WakeupTreeNode& n2) const
{
  return this->get_action() == n2.get_action();
}

bool WakeupTreeNode::is_contained_in(WakeupTreeNode& other_tree) const
{

  if (not have_same_content(other_tree))
    return false;

  for (auto& child : children_) {
    auto other_candidate = std::find_if(other_tree.children_.begin(), other_tree.children_.end(),
                                        [&](const auto& node) { return child->have_same_content(*node); });
    if (other_candidate == other_tree.children_.end())
      return false;
    if (not child->is_contained_in(**other_candidate))
      return false;
  }

  return true;
}

bool WakeupTree::is_contained_in(WakeupTree& other_tree) const
{
  if (root_ == nullptr)
    return true;
  if (other_tree.root_ == nullptr)
    return false; // if we are not empty but other is, we are not contained
  return this->root_->is_contained_in(*other_tree.root_);
}

void WakeupTree::force_insert(const PartialExecution& seq)
{
  WakeupTreeNode* cur_node = root_.get();
  for (const auto& w_i : seq) {
    if (auto node_after_aid = cur_node->get_node_after_actor(w_i->aid_); node_after_aid != nullptr) {
      cur_node = node_after_aid;
      continue;
    }

    std::unique_ptr<WakeupTreeNode> new_node = std::make_unique<WakeupTreeNode>(std::move(w_i));
    cur_node->add_child(std::move(new_node));
    cur_node = new_node.get();
  }
}

} // namespace simgrid::mc::odpor
