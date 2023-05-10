/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/WakeupTreeIterator.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"

namespace simgrid::mc::odpor {

WakeupTreeIterator::WakeupTreeIterator(const WakeupTree& tree)
{
  //   post_order_iteration.push(tree.root);
  push_until_left_most_found();
}

void WakeupTreeIterator::push_until_left_most_found()
{
  // INVARIANT: Since we are traversing over a tree,
  // there are no cycles. This means that at least
  // one node in the tree won't have any children,
  // so the loop will eventually terminate
  auto* cur_top_node = *post_order_iteration.top();
  while (!cur_top_node->is_leaf()) {
    // INVARIANT: Since we push children in
    // reverse order (right-most to left-most),
    // we ensure that we'll always process left-most
    // children first
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

  auto prev_top_handle = post_order_iteration.top();
  post_order_iteration.pop();

  // If there are now no longer any nodes left,
  // we know that `prev_top` must be the original
  // root; that is, we were *just* pointing at the
  // original root, so we're done
  if (post_order_iteration.empty()) {
    return;
  }

  // Otherwise, look at the next top node. If
  // `prev_top` is that node's right-most child,
  // then we don't attempt to re-add `next_top`'s
  // children again for we would have already seen them
  const auto* next_top_node = *post_order_iteration.top();

  // To actually determine "right-most", we check if
  // moving over to the right one spot brings us to the
  // end of the candidate parent's list
  if ((++prev_top_handle) != next_top_node->get_ordered_children().end()) {
    push_until_left_most_found();
  }
}

} // namespace simgrid::mc::odpor