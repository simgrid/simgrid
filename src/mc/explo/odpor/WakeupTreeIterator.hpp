/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_WAKEUP_TREE_ITERATOR_HPP
#define SIMGRID_MC_ODPOR_WAKEUP_TREE_ITERATOR_HPP

#include "src/mc/explo/odpor/odpor_forward.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <list>
#include <stack>

namespace simgrid::mc::odpor {

/**
 * @brief A forward-iterator that performs a postorder traversal
 * of the nodes of a WakeupTree
 *
 * Inserting a sequence `w` into a wakeup tree `B` with respect to
 * some execution `E` requires determining the "<-minimal" node `N`
 * with sequence `v` in the tree such that `v ~_[E] w`. The "<" relation
 * over a wakeup tree orders its nodes by first recursively ordering all
 * children of a node `N` followed by the node `N` itself, viz. a postorder.
 * This iterator provides such a postorder traversal over the nodes in the
 * wakeup tree.
 */
class WakeupTreeIterator
    : public boost::iterator_facade<WakeupTreeIterator, WakeupTreeNode*, boost::forward_traversal_tag> {
public:
  // Use rule-of-three, and implicitely disable the move constructor which cannot be 'noexcept' (as required by C++ Core
  // Guidelines), due to the std::list and std:stack<std::deque> members.
  WakeupTreeIterator()                          = default;
  WakeupTreeIterator(const WakeupTreeIterator&) = default;
  ~WakeupTreeIterator()                         = default;

  explicit WakeupTreeIterator(const WakeupTree& tree);

private:
  using node_handle = std::list<WakeupTreeNode*>::iterator;

  /**
   *  @brief A list which is used to "store" the root node of the traversed
   * wakeup tree
   *
   * The root node is, by definition, not the child of any other node. This
   * means that the root node also is contained in any list into which the
   * iterator can generate a pointer (iterator). This list takes the role
   * of allowing the iterator to treat the root node like any other.
   */
  std::list<WakeupTreeNode*> root_list;

  /**
   * @brief The current "view" of the iteration in post-order traversal
   */
  std::stack<node_handle> post_order_iteration;

  /**
   * @brief The nodes in the current ordering that have already
   * added their own children
   *
   * We need to be able to determine whether to add the children
   * of a given node. Eventually, we want to search that node itself,
   * but we have to first search its children. Later, when we
   * reach each node in this stack again, we'll remember not to add
   * its children and will search the node in the stack instead.
   */
  std::stack<WakeupTreeNode*> has_added_children;

  /**
   * @brief Search the wakeup tree until a leaf node appears at the front
   * of the iteration, pushing all children towards the top of the stack
   * as the search progresses
   */
  void push_until_left_most_found();

  // boost::iterator_facade<...> interface to implement
  void increment();
  bool equal(const WakeupTreeIterator& other) const { return post_order_iteration == other.post_order_iteration; }
  WakeupTreeNode*& dereference() const { return *post_order_iteration.top(); }

  // Allows boost::iterator_facade<...> to function properly
  friend class boost::iterator_core_access;
};

} // namespace simgrid::mc::odpor
#endif
