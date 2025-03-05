/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_WAKEUP_TREE_HPP
#define SIMGRID_MC_ODPOR_WAKEUP_TREE_HPP

#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/transition/Transition.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace simgrid::mc::odpor {

/** @brief Describes how a tree insertion was carried out */
enum class InsertionResult { leaf, interior_node, root };

/**
 * @brief A single node in a wakeup tree
 *
 * Each node in a wakeup tree represents a single step
 * taken in an extension of the execution represented
 * by the tree within which the node is contained. That is,
 * a node in the tree is one step on a "pre-defined"
 * path forward for some execution sequence. The partial
 * execution that is implicitly represented by the node
 * is that formed by taking each step on the (unique)
 * path in the tree from the root node to this node.
 * Thus, the tree itself contains all of the paths
 * that "should be" searched, while each node is
 * simply a step on each path.
 */
class WakeupTreeNode {
private:
  WakeupTreeNode* parent_ = nullptr;

  /** An ordered list of children of for this node in the tree */
  std::list<std::unique_ptr<WakeupTreeNode>> children_ = {};

  /** @brief The contents of the node */
  std::shared_ptr<Transition> action_ = nullptr;

  /** @brief Removes the node as a child from the parent */
  void detatch_from_parent();

  PartialExecution sequence_ = PartialExecution{};

  /** Allows the owning tree to insert directly into the child */
  friend WakeupTree;

public:
  explicit WakeupTreeNode(std::shared_ptr<Transition> u) : action_(u), sequence_({u}) {}

  WakeupTreeNode()                                 = default;
  ~WakeupTreeNode()                                = default;
  WakeupTreeNode(const WakeupTreeNode&)            = delete;
  WakeupTreeNode(WakeupTreeNode&&)                 = default;
  WakeupTreeNode& operator=(const WakeupTreeNode&) = delete;
  WakeupTreeNode& operator=(WakeupTreeNode&&)      = default;

  auto begin() const { return this->children_.begin(); }
  auto end() const { return this->children_.end(); }
  auto rbegin() const { return this->children_.rbegin(); }
  auto rend() const { return this->children_.rend(); }

  bool is_leaf() const { return children_.empty(); }
  bool is_root() const { return parent_ == nullptr; }
  aid_t get_actor() const { return action_->aid_; }
  const PartialExecution& get_sequence() const;

  /** @brief Return a shared pointer to the transition if the action exists.

   *  @note that the root of a wakeup tree does not correspond to any action.
   *  In that case, get_action() return nullptr.
   **/
  std::shared_ptr<Transition> get_action() const { return action_; }
  const std::list<std::unique_ptr<WakeupTreeNode>>& get_ordered_children() const { return children_; }

  std::string string_of_whole_tree(const std::string& prefix, bool is_first, bool is_last) const;

  /** Insert a node `node` as a new child of this node */
  void add_child(std::unique_ptr<WakeupTreeNode> node);

  /**
   * @brief returns true iff calling object is a subset of called object
   *
   */
  bool is_contained_in(WakeupTreeNode& other_tree) const;

  bool have_same_content(const WakeupTreeNode& n2) const;

  WakeupTreeNode* get_node_after_actor(aid_t aid) const
  {
    for (auto const& node : children_)
      if (node->get_actor() == aid)
        return node.get();

    return nullptr;
  }

  long get_size() const
  {
    long res = 0;
    for (auto const& child : children_) {
      res += 1;
      res += child->get_size();
    }
    return res;
  }

  // Does the actual job of recursively inserting a sequence inside a WuT.
  // The second version needs to return the node in order for the calling method to obtain
  // the inserted sequence.
  InsertionResult recursive_insert(WakeupTree& father, PartialExecution& w);
  WakeupTreeNode* recursive_insert_and_get_inserted_seq(WakeupTree& father, PartialExecution& w);
};

/**
 * @brief The structure used by ODPOR to maintains paths of execution
 * that should be followed in the future
 *
 * The wakeup tree data structure is formally defined in the Abdulla et al.
 * 2017 ODPOR paper. Conceptually, the tree consists of nodes which are
 * mapped to actions. Each node represents a partial extension of an execution,
 * the complete extension being the transitions taken in sequence from
 * the root of the tree to the node itself. Leaf nodes in the tree conceptually,
 * then, represent paths that are guaranteed to explore different parts
 * of the search space.
 *
 *
 * @note A wakeup tree is defined relative to some execution `E`. The
 * structure itself does not hold onto a reference of the execution with
 * respect to which it is a wakeup tree.
 *
 * @todo: If the idea of execution "views"  is ever added -- viz. being able
 * to share the contents of a single execution -- then a wakeup tree could
 * contain a reference to such a view which would then be maintained by the
 * manipulator of the tree
 */
class WakeupTree {
private:
  std::unique_ptr<WakeupTreeNode> root_;

  // Returns a pointer to the lastly inserted node
  WakeupTreeNode* insert_sequence_after(WakeupTreeNode* node, const PartialExecution& w);

  bool contains(const WakeupTreeNode* node) const;

public:
  WakeupTree();
  explicit WakeupTree(std::unique_ptr<WakeupTreeNode> root);

  /**
   * @brief extract the subtree after the left-most action
   */
  WakeupTree get_first_subtree();

  std::vector<std::string> get_single_process_texts() const;

  std::string string_of_whole_tree() const;

  void remove_subtree_at_aid(aid_t proc);

  /**
   * @brief Whether or not this tree is considered empty
   *
   * @note Unlike other collection types, a wakeup tree is
   * considered "empty" if it only contains the root node;
   * that is, if it is "uninteresting". In such a case,
   */
  bool empty() const { return root_->children_.size() == 0; }
  /**
   * @brief Gets the actor of the node that is the "smallest" (with respect
   * to the tree's "<" relation) single-process node.
   *
   * If the tree is empty, returns std::nullopt
   */
  std::optional<aid_t> get_min_single_process_actor() const;

  /**
   * @brief Gets the node itself that is the "smallest" (with respect
   * to the tree's "<" relation) single-process node.
   *
   * If the tree is empty, returns std::nullopt
   */
  std::optional<WakeupTreeNode*> get_min_single_process_node() const;

  void insert_at_root(std::shared_ptr<Transition> u);

  /**
   * @brief Inserts an sequence `seq` of processes into the tree
   * such that that this tree is a wakeup tree relative to the
   * given execution
   *
   * A key component of managing wakeup trees in ODPOR is
   * determining what should be inserted into a wakeup tree.
   * The procedure for implementing the insertion is outlined in section 6.2
   * of Abdulla et al. 2017 as follows:
   *
   * | Let `v` be the smallest (w.r.t to "<") sequence in [the tree] B
   * | such that `v ~_[E] w`. If `v` is a leaf node, the tree can be left
   * | unmodified.
   * |
   * | Otherwise let `w'` be the shortest sequence such that `w [=_[E] v.w'`
   * | and add `v.w'` as a new leaf, ordered after all already existing nodes
   * | of the form `v.w''`
   *
   * This method performs a recursive exploration of the existing tree. Compared
   * to the method proposed in the original paper, it is linear in the height of
   * the WuT and not in its size.
   *
   * @invariant: It is assumed that this tree is a wakeup tree
   * with respect to the given execution `E`
   *
   * @return Whether a sequence equivalent to `seq` is already contained
   * as a leaf node in the tree
   */
  InsertionResult insert(const PartialExecution& seq);

  /**
   * @brief Does the same as 'insert' but instead of returning a result type, yield the
   * inserted sequence.
   */
  const PartialExecution insert_and_get_inserted_seq(const PartialExecution& seq);

  /**
   * @brief The number of children at depth one
   */
  unsigned int count_direct_children() const { return root_->get_ordered_children().size(); }

  std::vector<aid_t> get_direct_children_actors() const
  {
    std::vector<aid_t> result;
    for (auto const& leaf : root_->get_ordered_children())
      result.push_back(leaf->get_actor());
    return result;
  }

  /**
   * @brief Gets the node itself that is the the one at the root directly after
   * aid.
   *
   * If the tree is empty, returns nullptr
   */
  WakeupTreeNode* get_node_after_actor(aid_t aid) const;

  /**
   * @brief returns true iff calling object is a subset of called object
   *
   */
  bool is_contained_in(WakeupTree& other_tree) const;

  /**
   * @brief insert a sequence in the wakeup tree as though it was a normal tree.
   *
   * In particular, this won't use the normal equivalence method. You shouldn't be
   * calling this method, unless you are using another wakeup tree anyway.
   * (see BFSWutstate for more information)
   *
   */
  void force_insert(const PartialExecution& seq);

  long get_size() const
  {
    long res = 1;
    for (auto const& child : root_->children_) {
      res += 1;
      res += child->get_size();
    }
    return res;
  }

  friend WakeupTreeNode;
};

} // namespace simgrid::mc::odpor
#endif
