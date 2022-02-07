/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/Transition.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/Session.hpp"
#include "xbt/asserts.h"

namespace simgrid {
namespace mc {
unsigned long Transition::executed_transitions_ = 0;

std::string Transition::to_string()
{
  xbt_assert(mc_model_checker != nullptr, "Must be called from MCer");

  return textual_;
}
RemotePtr<simgrid::kernel::actor::SimcallObserver> Transition::execute()
{
  textual_ = mc_model_checker->simcall_to_string(aid_, times_considered_);
  executed_transitions_++;
  return session_singleton->execute(*this);
}
} // namespace mc
} // namespace simgrid
