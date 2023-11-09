/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_WAKEUP_TREE_HPP
#define SIMGRID_MC_ODPOR_WAKEUP_TREE_HPP

#include "src/mc/explo/odpor/WakeupTreeIterator.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/transition/Transition.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace simgrid::mc::odpor {

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
  std::list<WakeupTreeNode*> children_;

  /** @brief The contents of the node */
  std::shared_ptr<Transition> action_;

  /** @brief Removes the node as a child from the parent */
  void detatch_from_parent();

  /** Allows the owning tree to insert directly into the child */
  friend WakeupTree;
  friend WakeupTreeIterator;

public:
  explicit WakeupTreeNode(std::shared_ptr<Transition> u) : action_(u) {}

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
  PartialExecution get_sequence() const;
  std::shared_ptr<Transition> get_action() const { return action_; }
  const std::list<WakeupTreeNode*>& get_ordered_children() const { return children_; }

  std::string string_of_whole_tree(int indentation_level) const;

  /** Insert a node `node` as a new child of this node */
  void add_child(WakeupTreeNode* node);
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
 * Iteration over a wakeup tree occurs as a post-order traversal of its nodes
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
  WakeupTreeNode* root_;

  /**
   * @brief All of the nodes that are currently are a part of the tree
   *
   * @invariant Each node event maps itself to the owner of that node,
   * i.e. the unique pointer that manages the data at the address. The tree owns all
   * of the addresses that are referenced by the nodes WakeupTreeNode.
   * ODPOR guarantees that nodes are persisted as long as needed.
   */
  std::unordered_map<WakeupTreeNode*, std::unique_ptr<WakeupTreeNode>> nodes_;

  void insert_node(std::unique_ptr<WakeupTreeNode> node);
  void insert_sequence_after(WakeupTreeNode* node, const PartialExecution& w);
  void remove_node(WakeupTreeNode* node);
  bool contains(const WakeupTreeNode* node) const;

  /**
   * @brief Removes the node `root` and all of its descendants from
   * this wakeup tree
   *
   * @throws: If the node `root` is not contained in this tree, an
   * exception is raised
   */
  void remove_subtree_rooted_at(WakeupTreeNode* root);

  /**
   * @brief Adds a new node to the tree, disconnected from
   * any other, which represents the partial execution
   * "fragment" `u`
   */
  WakeupTreeNode* make_node(std::shared_ptr<Transition> u);

  /* Allow the iterator to access the contents of the tree */
  friend WakeupTreeIterator;

public:
  WakeupTree();
  explicit WakeupTree(std::unique_ptr<WakeupTreeNode> root);

  /**
   * @brief Creates a copy of the subtree whose root is the node
   * `root` in this tree
   */
  static WakeupTree make_subtree_rooted_at(WakeupTreeNode* root);

  auto begin() const { return WakeupTreeIterator(*this); }
  auto end() const { return WakeupTreeIterator(); }

  std::vector<std::string> get_single_process_texts() const;

  std::string string_of_whole_tree() const;

  /**
   * @brief Remove the subtree of the smallest (with respect
   * to the tree's "<" relation) single-process node.
   *
   * A "single-process" node is one whose execution represents
   * taking a single action (i.e. those of the root node). The
   * smallest under "<" is that which is continuously selected and
   * removed by ODPOR.
   *
   * If the tree is empty, this method has no effect.
   */
  void remove_min_single_process_subtree();

  void remove_subtree_at_aid(aid_t proc);

  /**
   * @brief Whether or not this tree is considered empty
   *
   * @note Unlike other collection types, a wakeup tree is
   * considered "empty" if it only contains the root node;
   * that is, if it is "uninteresting". In such a case,
   */
  bool empty() const { return nodes_.size() == static_cast<size_t>(1); }

  /**
   * @brief Returns the number of *non-empty* entries in the tree, viz. the
   * number of nodes in the tree that have an action mapped to them
   */
  size_t get_num_entries() const { return not empty() ? (nodes_.size() - 1) : static_cast<size_t>(0); }

  /**
   * @brief Returns the number of nodes in the tree, including the root node
   */
  size_t get_num_nodes() const { return nodes_.size(); }

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

  /** @brief Describes how a tree insertion was carried out */
  enum class InsertionResult { leaf, interior_node, root };

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
   * This method performs the post-order search of part one and the insertion of
   * `v.w'` of part two of the above procedure. Note that the execution will
   * provide `v.w'` (see `Execution::get_shortest_odpor_sq_subset_insertion()`).
   *
   * @invariant: It is assumed that this tree is a wakeup tree
   * with respect to the given execution `E`
   *
   * @return Whether a sequence equivalent to `seq` is already contained
   * as a leaf node in the tree
   */
  InsertionResult insert(const Execution& E, const PartialExecution& seq);
};

} // namespace simgrid::mc::odpor
#endif
