/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionAny.hpp"
#include "xbt/asserts.h"
#include <simgrid/config.h>
#if SIMGRID_HAVE_MC
#include "src/mc/ModelChecker.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/api/State.hpp"
#endif

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_any, mc_transition, "Logging specific to MC WaitAny / TestAny transitions");

namespace simgrid {
namespace mc {

TestAnyTransition::TestAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::TESTANY, issuer, times_considered)
{
  int size;
  xbt_assert(stream >> size);
  for (int i = 0; i < size; i++) {
    Transition* t = deserialize_transition(issuer, 0, stream);
    XBT_DEBUG("TestAny received a transition %s", t->to_string(true).c_str());
    transitions_.push_back(t);
  }
}
std::string TestAnyTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("%ld: TestAny{ ", aid_);
  for (auto const* t : transitions_)
    res += t->to_string(verbose);
  res += "}";
  return res;
}
bool TestAnyTransition::depends(const Transition* other) const
{
  return transitions_[times_considered_]->depends(other);
}
WaitAnyTransition::WaitAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::WAITANY, issuer, times_considered)
{
  int size;
  xbt_assert(stream >> size);
  for (int i = 0; i < size; i++) {
    Transition* t = deserialize_transition(issuer, 0, stream);
    transitions_.push_back(t);
  }
}
std::string WaitAnyTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("%ld: WaitAny{ ", aid_);
  for (auto const* t : transitions_)
    res += t->to_string(verbose);
  res += "}";
  return res;
}
bool WaitAnyTransition::depends(const Transition* other) const
{
  return transitions_[times_considered_]->depends(other);
}

} // namespace mc
} // namespace simgrid
