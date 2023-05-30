/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionActorJoin.hpp"
#include "simgrid/config.h"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_actorlifecycle, mc_transition,
                                "Logging specific to MC transitions about actors' lifecycle: joining, ending");

namespace simgrid::mc {

ActorJoinTransition::ActorJoinTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::ACTOR_JOIN, issuer, times_considered)
{
  xbt_assert(stream >> target_ >> timeout_);
  XBT_DEBUG("ActorJoinTransition target:%ld, %s ", target_, (timeout_ ? "timeout" : "no-timeout"));
}
std::string ActorJoinTransition::to_string(bool verbose) const
{
  return xbt::string_printf("ActorJoin(target %ld, %s)", target_, (timeout_ ? "timeout" : "no timeout"));
}
bool ActorJoinTransition::depends(const Transition* other) const
{
  // Joining is dependent with any transition whose
  // actor is that of the `other` action. , Join i
  if (other->aid_ == target_) {
    return true;
  }

  // Actions executed by the same actor are always dependent
  if (other->aid_ == aid_)
    return true;

  // Otherwise, joining is indep with any other transitions:
  // - It is only enabled once the target ends, and after this point it's enabled no matter what
  // - Other joins don't affect it, and it does not impact on the enabledness of any other transition
  return false;
}

} // namespace simgrid::mc
