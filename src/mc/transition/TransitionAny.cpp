/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionAny.hpp"
#include "simgrid/config.h"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_any, mc_transition, "Logging specific to MC WaitAny / TestAny transitions");

namespace simgrid::mc {

TestAnyTransition::TestAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::TESTANY, issuer, times_considered)
{
  int size;
  xbt_assert(stream >> size);
  for (int i = 0; i < size; i++) {
    Transition* t = deserialize_transition(issuer, 0, stream);
    XBT_INFO("TestAny received a transition %s", t->to_string(true).c_str());
    transitions_.push_back(t);
  }
}
std::string TestAnyTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("TestAny(%s){ ", this->result() ? "TRUE" : "FALSE");
  for (auto const* t : transitions_) {
    res += t->to_string(verbose);
    res += "; ";
  }
  res += " }";
  return res;
}
bool TestAnyTransition::depends(const Transition* other) const
{
  // Actions executed by the same actor are always dependent
  if (other->aid_ == aid_)
    return true;

  return transitions_[times_considered_]->depends(other);
}
bool TestAnyTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::TESTANY, "Unexpected transition type %s", to_c_str(type_));

  return true; // TestAny is always enabled
}

WaitAnyTransition::WaitAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::WAITANY, issuer, times_considered)
{
  int size;
  xbt_assert(stream >> size);
  for (int i = 0; i < size; i++) {
    Transition* t = deserialize_transition(issuer, 0, stream);
    XBT_INFO("WaitAny received transition %d/%d %s", (i + 1), size, t->to_string(true).c_str());
    transitions_.push_back(t);
  }
}
std::string WaitAnyTransition::to_string(bool verbose) const
{
  auto res = xbt::string_printf("WaitAny{ ");
  for (auto const* t : transitions_)
    res += t->to_string(verbose);
  res += " } (times considered = " + std::to_string(times_considered_) + ")";
  return res;
}
bool WaitAnyTransition::depends(const Transition* other) const
{
  // Actions executed by the same actor are always dependent
  if (other->aid_ == aid_)
    return true;
  return transitions_[times_considered_]->depends(other);
}
bool WaitAnyTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::WAITANY, "Unexpected transition type %s", to_c_str(type_));

  // TODO: We need to check if any of the transitions waited on occurred before `e1`
  return true; // Let's overapproximate to not miss branches
}

} // namespace simgrid::mc
