/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/Transition.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"
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

// Do not move this to the header, to ensure that we have a vtable for Transition
Transition::~Transition() = default;

std::string Transition::to_string(bool)
{
  return textual_;
}
const char* Transition::to_cstring(bool verbose)
{
  to_string(verbose);
  return textual_.c_str();
}
void Transition::replay() const
{
  replayed_transitions_++;

#if SIMGRID_HAVE_MC
  mc_model_checker->handle_simcall(aid_, times_considered_, false);
  mc_model_checker->wait_for_requests();
#endif
}
std::string RandomTransition::to_string(bool verbose)
{
  return xbt::string_printf("Random([%d;%d] ~> %d)", min_, max_, times_considered_);
}

RandomTransition::RandomTransition(aid_t issuer, int times_considered, char* buffer)
    : Transition(issuer, times_considered)
{
  std::stringstream stream(buffer);
  stream >> min_ >> max_;
}

} // namespace mc
} // namespace simgrid
