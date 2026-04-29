/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionActor.hpp"
#include "simgrid/config.h"
#include "src/mc/remote/Channel.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_actorlifecycle, mc_transition,
                                "Logging specific to MC transitions about actors' lifecycle: joining, ending");

namespace simgrid::mc {

ActorJoinTransition::ActorJoinTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::ACTOR_JOIN, issuer, times_considered)
{
  target_  = channel.unpack<aid_t>();
  timeout_ = channel.unpack<bool>();
  XBT_DEBUG("ActorJoinTransition target:%ld, %s ", target_, (timeout_ ? "timeout" : "no-timeout"));
}
std::string ActorJoinTransition::to_string(bool verbose) const
{
  return xbt::string_printf("ActorJoin(target %ld, %s)", target_, (timeout_ ? "timeout" : "no timeout"));
}

bool ActorJoinTransition::can_be_co_enabled(const Transition* other) const
{
  if (other->type_ < type_)
    return other->can_be_co_enabled(this);

  // Transitions of a same actor have no chance at being co-enabled
  if (other->aid_ == aid_)
    return false;

  // An actor join isn't enabled if the target can still act
  if (other->aid_ == target_)
    return false;

  return true;
}

bool ActorJoinTransition::reversible_race(const Transition* other, const odpor::Execution* exec,
                                          EventHandle this_handle, EventHandle other_handle) const
{
  xbt_assert(type_ == Type::ACTOR_JOIN, "Unexpected transition type %s", to_c_str(type_));

  // ActorJoin races with another event iff its target `T` is the same as  the actor executing the other transition.
  // Clearly, then, we could not join on that actor `T` and then run a transition by `T`, so no race is reversible
  return false;
}
/* --------------------- */
/* This action is a no-op and could be explored in most cases, but it's useful when observing the memory accesses, as
   the vector of memaccesses is sent with the following simcall. If this action were omited, the last mem accesses of
   the actors would not get observed by the checker. See this as a placeholder intended to carry the trailing memory
   accesses */
ActorExitTransition::ActorExitTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::ACTOR_EXIT, issuer, times_considered)
{
  XBT_DEBUG("ActorExitTransition ");
}
std::string ActorExitTransition::to_string(bool verbose) const
{
  return xbt::string_printf("ActorExit()");
}

bool ActorExitTransition::can_be_co_enabled(const Transition* other) const
{
  if (other->type_ < type_)
    return other->can_be_co_enabled(this);

  // Transitions of a same actor have no chance at being co-enabled
  if (other->aid_ == aid_)
    return false;

  return true;
}

bool ActorExitTransition::reversible_race(const Transition* other, const odpor::Execution* exec,
                                          EventHandle this_handle, EventHandle other_handle) const
{
  // ActorExit races only with JOIN or CREATE, and then, they are not reversible.
  return false;
}
/* --------------------- */
ActorSleepTransition::ActorSleepTransition(aid_t issuer, int times_considered, mc::Channel&)
    : Transition(Type::ACTOR_SLEEP, issuer, times_considered)
{
  XBT_DEBUG("ActorSleepTransition()");
}
std::string ActorSleepTransition::to_string(bool verbose) const
{
  return xbt::string_printf("ActorSleep()");
}

bool ActorSleepTransition::reversible_race(const Transition* other, const odpor::Execution* exec,
                                           EventHandle this_handle, EventHandle other_handle) const
{
  xbt_assert(other->type_ == Type::ACTOR_CREATE, "Unexpected transition type %s", to_c_str(type_));

  return false; // The creation of the actor is the only way to be dependent with a sleep
}

ActorCreateTransition::ActorCreateTransition(aid_t issuer, int times_considered, mc::Channel& channel)
    : Transition(Type::ACTOR_CREATE, issuer, times_considered)
{
  child_ = channel.unpack<aid_t>();
  XBT_DEBUG("ActorCreateTransition child:%ld", child_);
}
std::string ActorCreateTransition::to_string(bool verbose) const
{
  return xbt::string_printf("ActorCreate(child %ld)", child_);
}

bool ActorCreateTransition::can_be_co_enabled(const Transition* other) const
{
  if (other->type_ < type_)
    return other->can_be_co_enabled(this);

  // Transitions of a same actor have no chance at being co-enabled
  if (other->aid_ == aid_)
    return false;

  // An actor join isn't enabled if the target can still act
  if (other->aid_ == child_)
    return false;

  return true;
}

bool ActorCreateTransition::reversible_race(const Transition* other, const odpor::Execution* exec,
                                            EventHandle this_handle, EventHandle other_handle) const
{
  // FIXME: Please review this code, MLaurent :)
  xbt_assert(type_ == Type::ACTOR_CREATE, "Unexpected transition type %s", to_c_str(type_));

  // ActorJoin races with another event iff its target `T` is the same as  the actor executing the other transition.
  // Clearly, then, we could not join on that actor `T` and then run a transition by `T`, so no race is reversible
  return false;
}

} // namespace simgrid::mc
