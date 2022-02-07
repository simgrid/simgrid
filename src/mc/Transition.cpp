/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/Transition.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/mc_state.hpp"
#include "src/mc/remote/RemoteProcess.hpp"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_transition, mc, "Logging specific to MC transitions");

namespace simgrid {
namespace mc {
unsigned long Transition::executed_transitions_ = 0;

std::string Transition::to_string() const
{
  xbt_assert(mc_model_checker != nullptr, "Must be called from MCer");

  return textual_;
}
const char* Transition::to_cstring() const
{
  xbt_assert(mc_model_checker != nullptr, "Must be called from MCer");

  return textual_.c_str();
}
void Transition::init(aid_t aid, int times_considered)
{
  aid_              = aid;
  times_considered_ = times_considered;
  textual_ = mc_model_checker->simcall_to_string(aid_, times_considered_);
}
RemotePtr<simgrid::kernel::actor::SimcallObserver> Transition::replay() const
{
  executed_transitions_++;

  simgrid::mc::RemotePtr<simgrid::kernel::actor::SimcallObserver> res = mc_model_checker->handle_simcall(*this);
  mc_model_checker->wait_for_requests();

  return res;
}

} // namespace mc
} // namespace simgrid
