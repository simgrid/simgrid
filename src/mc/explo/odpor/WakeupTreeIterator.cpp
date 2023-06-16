/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/WakeupTreeIterator.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "xbt/asserts.h"

namespace simgrid::mc::odpor {

WakeupTreeIterator::WakeupTreeIterator(const WakeupTree& tree) : root_list{tree.root_}
{
  post_order_iteration.push(root_list.begin());
  push_until_left_most_found();
}

void WakeupTreeIterator::push_until_left_most_found()
{
  // INVARIANT: Since we are traversing over a tree,
  // there are no cycles. This means that at least
  // one node in the tree won't have any children,
  // so the loop will eventually terminate
  WakeupTreeNode* cur_top_node = *post_order_iteration.top();
  while (not cur_top_node->is_leaf()) {
    // INVARIANT: Since we push children in
    // reverse order (right-most to left-most),
    // we ensure that we'll always process left-most
    // children first
    auto& children = cur_top_node->children_;
    for (auto iter = children.rbegin(); iter != children.rend(); ++iter) {
      // iter.base() points one element past where we seek; that is,
      // we want the value one position forward
      post_order_iteration.push(std::prev(iter.base()));
    }
    has_added_children.push(cur_top_node);
    cur_top_node = *post_order_iteration.top();
  }
}

void WakeupTreeIterator::increment()
{
  // If there are no nodes in the stack, we've
  // completed the traversal: there's nothing left
  // to do
  if (post_order_iteration.empty()) {
    return;
  }

  post_order_iteration.pop();

  // If there are now no longer any nodes left,
  // we know that `prev_top` must be the original
  // root; that is, we were *just* pointing at the
  // original root, so we're done
  if (post_order_iteration.empty()) {
    return;
  }

  xbt_assert(not has_added_children.empty(), "Invariant violated: There are more "
                                             "nodes in the iteration that we must search "
                                             "yet nobody has claimed to have added these nodes. "
                                             "This implies that the algorithm is not iterating over "
                                             "the wakeup tree is not following the post-fix order "
                                             "correctly");

  // Otherwise, look at what is the new, current top node.
  // We're traversing the tree in
  //
  // If we've already added our children, we want
  // to be sure not to add them again; but we ALSO
  // want to be sure that we now start checking against
  // the the node that's next in line as "finished"
  //
  // INVARIANT: Since we're searching in post-fix order,
  // it always suffices to compare the current node
  // with the top of the stack of nodes which have added their
  // children
  if (*post_order_iteration.top() == has_added_children.top()) {
    has_added_children.pop();
  } else {
    push_until_left_most_found();
  }
}

} // namespace simgrid::mc::odpor