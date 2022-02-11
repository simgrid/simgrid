/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/Transition.hpp"
#include "xbt/asserts.h"
#include <simgrid/config.h>
#if SIMGRID_HAVE_MC
#include "src/mc/ModelChecker.hpp"
#endif

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_transition, mc, "Logging specific to MC transitions");

namespace simgrid {
namespace mc {
unsigned long Transition::executed_transitions_ = 0;
unsigned long Transition::replayed_transitions_ = 0;

Transition::~Transition() {
} // Make sure that we have a vtable for Transition by putting this virtual function out of the header

std::string Transition::to_string(bool verbose)
{
  return textual_;
}
const char* Transition::to_cstring(bool verbose)
{
  to_string();
  return textual_.c_str();
}
void Transition::init(aid_t aid, int times_considered)
{
  aid_              = aid;
  times_considered_ = times_considered;
}
void Transition::replay() const
{
  replayed_transitions_++;

#if SIMGRID_HAVE_MC
  mc_model_checker->handle_simcall(*this, false);
  mc_model_checker->wait_for_requests();
#endif
}

} // namespace mc
} // namespace simgrid
