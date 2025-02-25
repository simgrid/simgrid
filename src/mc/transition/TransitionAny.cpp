/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionAny.hpp"
#include "simgrid/config.h"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_any, mc_transition, "Logging specific to MC WaitAny / TestAny transitions");

namespace simgrid::mc {

TestAnyTransition::TestAnyTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::TESTANY, issuer, times_considered)
{
  unsigned size = channel.unpack<unsigned>();
  for (unsigned i = 0; i < size; i++) {
    Transition* t = deserialize_transition(issuer, 0, channel);
    XBT_DEBUG("TestAny received transition %u/%u %s", (i + 1), size, t->to_string(true).c_str());
    transitions_.push_back(t);
  }
  call_location_ = channel.unpack<std::string>();
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
  for (auto const& transition : transitions_)
    if (transition->depends(other))
      return true;
  return false;
}
bool TestAnyTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::TESTANY, "Unexpected transition type %s", to_c_str(type_));

  return true; // TestAny is always enabled
}

WaitAnyTransition::WaitAnyTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::WAITANY, issuer, times_considered)
{
  unsigned size = channel.unpack<unsigned>();
  for (unsigned i = 0; i < size; i++) {
    Transition* t = deserialize_transition(issuer, 0, channel);
    XBT_DEBUG("WaitAny received transition %u/%u %s", (i + 1), size, t->to_string(true).c_str());
    transitions_.push_back(t);
  }
  call_location_ = channel.unpack<std::string>();
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
  for (auto const& transition : transitions_)
    if (transition->depends(other))
      return true;
  return false;
}
bool WaitAnyTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::WAITANY, "Unexpected transition type %s", to_c_str(type_));

  for (auto const& transition : transitions_)
    if (transition->reversible_race(other))
      return true; // Let's overapproximate to not miss branches, if we can reverse any transition, let's go for it

  return false;
}

} // namespace simgrid::mc
