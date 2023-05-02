/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/Execution.hpp"

namespace simgrid::mc::odpor {

std::unordered_set<Execution::EventHandle> Execution::get_racing_events_of(Execution::EventHandle e1) const
{
  return {};
}

bool Execution::happens_before(Execution::EventHandle e1, Execution::EventHandle e2) const
{
  return true;
}

Execution Execution::get_prefix_up_to(Execution::EventHandle)
{
  return *this;
}

void Execution::pop_latest() {}

void Execution::push_transition(const Transition*) {}

} // namespace simgrid::mc::odpor