/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_WAKEUP_TREE_ITERATOR_HPP
#define SIMGRID_MC_ODPOR_WAKEUP_TREE_ITERATOR_HPP

#include "src/mc/explo/odpor/WakeupTree.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <list>
#include <stack>

namespace simgrid::mc::odpor {

struct WakeupTreeIterator
    : public boost::iterator_facade<WakeupTreeIterator, const WakeupTreeNode*, boost::forward_traversal_tag> {
public:
  WakeupTreeIterator() = default;
  explicit WakeupTreeIterator(const WakeupTree& tree);

private:
  using node_handle = std::list<const WakeupTreeNode*>::iterator;

  /**
   * @brief The current "view" of the iteration in post-order traversal
   */
  std::stack<node_handle> post_order_iteration;

  /**
   *
   */
  void push_until_left_most_found();

  // boost::iterator_facade<...> interface to implement
  void increment();
  bool equal(const WakeupTreeIterator& other) const { return post_order_iteration == other.post_order_iteration; }
  const WakeupTreeNode*& dereference() const { return *post_order_iteration.top(); }

  // Allows boost::iterator_facade<...> to function properly
  friend class boost::iterator_core_access;
};

} // namespace simgrid::mc::odpor
#endif
