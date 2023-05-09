/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/odpor/Execution.hpp"

#include <algorithm>

namespace simgrid::mc::odpor {

WakeupTree::WakeupTree() : root(nullptr) {}

void WakeupTree::insert(const Execution& E, const ExecutionSequence& w)
{
  // Find the first node `v` in the tree such that
  // `v ~_[E] w`
}

} // namespace simgrid::mc::odpor