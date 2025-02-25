/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionSynchro.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/transition/TransitionActor.hpp"
#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "xbt/asserts.h"
#include "xbt/ex.h"
#include "xbt/string.hpp"

#include <inttypes.h>
#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_synchro, mc_transition, "Logging specific to MC synchronization transitions");

namespace simgrid::mc {

std::string BarrierTransition::to_string(bool verbose) const
{
  return xbt::string_printf("%s(barrier: %u)", Transition::to_c_str(type_), bar_);
}
BarrierTransition::BarrierTransition(aid_t issuer, int times_considered, Type type, mc::Channel& channel)
    : Transition(type, issuer, times_considered)
{
  bar_ = channel.unpack<unsigned>();
}
bool BarrierTransition::depends(const Transition* o) const
{
  if (o->type_ < type_)
    return o->depends(this);

  // Actions executed by the same actor are always dependent
  if (o->aid_ == aid_)
    return true;

  if (const auto* other = dynamic_cast<const BarrierTransition*>(o)) {
    if (bar_ != other->bar_)
      return false;

    // LOCK indep LOCK: requests are not ordered in a barrier
    if (type_ == Type::BARRIER_ASYNC_LOCK && other->type_ == Type::BARRIER_ASYNC_LOCK)
      return false;

    // WAIT indep WAIT: requests are not ordered
    if (type_ == Type::BARRIER_WAIT && other->type_ == Type::BARRIER_WAIT)
      return false;

    return true; // LOCK/WAIT is dependent because lock may enable wait
  }

  return false; // barriers are INDEP with non-barrier transitions
}
bool BarrierTransition::reversible_race(const Transition* other) const
{
  if (other->type_ == Type::ACTOR_CREATE) {
    const ActorCreateTransition* ctransition = static_cast<const ActorCreateTransition*>(other);
    xbt_assert(aid_ == ctransition->get_child());
    return false;
  }

  switch (type_) {
    case Type::BARRIER_ASYNC_LOCK:
      return true; // BarrierAsyncLock is always enabled
    case Type::BARRIER_WAIT:
      // If the other event is a barrier lock event, then we are not reversible;
      // otherwise we are reversible.
      return other->type_ != Transition::Type::BARRIER_ASYNC_LOCK;
    default:
      xbt_die("Unexpected transition type %s", to_c_str(type_));
  }
}

std::string MutexTransition::to_string(bool verbose) const
{
  return xbt::string_printf("%s(mutex: %" PRIxPTR ", owner: %ld)", Transition::to_c_str(type_), mutex_, owner_);
}

MutexTransition::MutexTransition(aid_t issuer, int times_considered, Type type, mc::Channel& channel)
    : Transition(type, issuer, times_considered)
{
  mutex_ = channel.unpack<unsigned>();
  owner_ = channel.unpack<aid_t>();
}

bool MutexTransition::depends(const Transition* o) const
{
  if (o->type_ < type_)
    return o->depends(this);

  // Actions executed by the same actor are always dependent
  if (o->aid_ == aid_)
    return true;

  // type_ <= other->type_ in  MUTEX_LOCK, MUTEX_TEST, MUTEX_TRYLOCK, MUTEX_UNLOCK, MUTEX_WAIT,

  // Theorem 4.4.11: LOCK indep TEST/WAIT.
  //  If both enabled, the result does not depend on their order. If WAIT is not enabled, LOCK won't enable it.
  if (type_ == Type::MUTEX_ASYNC_LOCK && (o->type_ == Type::MUTEX_TEST || o->type_ == Type::MUTEX_WAIT))
    return false;

  // Theorem 4.4.8: LOCK indep UNLOCK.
  //  pop_front and push_back are independent.
  if (type_ == Type::MUTEX_ASYNC_LOCK && o->type_ == Type::MUTEX_UNLOCK)
    return false;

  // Theorem 4.4.9: LOCK indep UNLOCK.
  //  any combination of wait and test is indenpendent.
  if ((type_ == Type::MUTEX_WAIT || type_ == Type::MUTEX_TEST) &&
      (o->type_ == Type::MUTEX_WAIT || o->type_ == Type::MUTEX_TEST))
    return false;

  // TEST is a pure function; TEST/WAIT won't change the owner; TRYLOCK will always fail if TEST is enabled (because a
  // request is queued)
  if (type_ == Type::MUTEX_TEST &&
      (o->type_ == Type::MUTEX_TEST || o->type_ == Type::MUTEX_TRYLOCK || o->type_ == Type::MUTEX_WAIT))
    return false;

  // TRYLOCK will always fail if TEST is enabled (because a request is queued), and may not overpass the WAITed
  // request in the queue
  if (type_ == Type::MUTEX_TRYLOCK && o->type_ == Type::MUTEX_WAIT)
    return false;

  // We are not considering the contextual dependency saying that UNLOCK is indep with WAIT/TEST
  // iff wait/test are not first in the waiting queue, because the other WAIT are not enabled anyway so this optim is
  // useless

  // two unlock can never occur in the same state, or after one another. Hence, the independency is true by
  // verifying a forall on an empty set.
  if (type_ == Type::MUTEX_UNLOCK && o->type_ == Type::MUTEX_UNLOCK)
    return false;

  // An async_lock is behaving as a mutex_unlock, so it muste have the same behavior regarding Mutex Wait
  if (type_ == Type::MUTEX_WAIT && o->type_ == Type::CONDVAR_ASYNC_LOCK)
    return mutex_ == static_cast<const CondvarTransition*>(o)->get_mutex();

  // Theorem 4.4.7: Any pair of synchronization actions of distinct actors concerning distinct mutexes are independent
  // Since it's the last rule in this file, we can use the contrapositive version of the theorem
  if (o->type_ == Type::MUTEX_ASYNC_LOCK || o->type_ == Type::MUTEX_TEST || o->type_ == Type::MUTEX_TRYLOCK ||
      o->type_ == Type::MUTEX_UNLOCK || o->type_ == Type::MUTEX_WAIT)
    return (mutex_ == static_cast<const MutexTransition*>(o)->mutex_);

  return false;
}

bool MutexTransition::can_be_co_enabled(const Transition* o) const
{
  if (o->type_ < type_)
    return o->can_be_co_enabled(this);

  // Transition executed by the same actor can never be co-enabled
  if (o->aid_ == aid_)
    return false;

  // type_ <= other->type_ in  MUTEX_LOCK, MUTEX_TEST, MUTEX_TRYLOCK, MUTEX_UNLOCK, MUTEX_WAIT,

  if (o->type_ == Type::MUTEX_ASYNC_LOCK || o->type_ == Type::MUTEX_TEST || o->type_ == Type::MUTEX_TRYLOCK ||
      o->type_ == Type::MUTEX_UNLOCK || o->type_ == Type::MUTEX_WAIT) {

    // No interaction between two different mutexes
    if (mutex_ != static_cast<const MutexTransition*>(o)->mutex_)
      return true;

  } // mutex transition

    // If someone can unlock, that someon has the mutex. Hence, nobody can wait on it
  if (type_ == Type::MUTEX_UNLOCK && o->type_ == Type::MUTEX_WAIT)
    return false;

  // If someone can wait, that someone has the mutex. Hence, nobody else can wait on it
  if (type_ == Type::MUTEX_WAIT && o->type_ == Type::MUTEX_WAIT)
    return false;

  // If you can wait on the mutex, then the CONDVAR async lock cannot be enabled
  if (type_ == Type::MUTEX_WAIT && o->type_ == Type::CONDVAR_ASYNC_LOCK)
    return mutex_ != static_cast<const CondvarTransition*>(o)->get_mutex();

  return true; // mutexes are INDEP with non-mutex transitions
}

bool SemaphoreTransition::reversible_race(const Transition* other) const
{
  if (other->type_ == Type::ACTOR_CREATE) {
    const ActorCreateTransition* ctransition = static_cast<const ActorCreateTransition*>(other);
    xbt_assert(aid_ == ctransition->get_child());
    return false;
  }

  if (other->type_ == Type::ACTOR_JOIN) {
    const ActorJoinTransition* jtransition = static_cast<const ActorJoinTransition*>(other);
    xbt_assert(aid_ == jtransition->get_target());
    return false;
  }

  switch (type_) {
    case Type::SEM_ASYNC_LOCK:
      return true; // SemAsyncLock is always enabled
    case Type::SEM_UNLOCK:
      return true; // SemUnlock is always enabled
    case Type::SEM_WAIT:
      // Some times the race is not reversible: we decide to catch it during the exploration
      // instead of doing tedious computation here.
      return true;
    default:
      xbt_die("Unexpected transition type %s", to_c_str(type_));
  }
}

std::string SemaphoreTransition::to_string(bool verbose) const
{
  if (type_ == Type::SEM_ASYNC_LOCK || type_ == Type::SEM_UNLOCK)
    return xbt::string_printf("%s(semaphore: %u, capacity: %u)", Transition::to_c_str(type_), sem_, capacity_);
  if (type_ == Type::SEM_WAIT)
    return xbt::string_printf("%s(semaphore: %u, capacity: %u, granted: %s)", Transition::to_c_str(type_), sem_,
                              capacity_, granted_ ? "yes" : "no");
  THROW_IMPOSSIBLE;
}
SemaphoreTransition::SemaphoreTransition(aid_t issuer, int times_considered, Type type, mc::Channel& channel)
    : Transition(type, issuer, times_considered)
{
  sem_      = channel.unpack<unsigned>();
  granted_  = channel.unpack<bool>();
  capacity_ = channel.unpack<unsigned>();
}
bool SemaphoreTransition::depends(const Transition* o) const
{
  if (o->type_ < type_)
    return o->depends(this);

  // Actions executed by the same actor are always dependent
  if (o->aid_ == aid_)
    return true;

  // LOCK indep UNLOCK: pop_front and push_back are independent.
  if (type_ == Type::SEM_ASYNC_LOCK && o->type_ == Type::SEM_UNLOCK)
    return false;

  // LOCK indep WAIT: If both enabled, ordering has no impact on the result. If WAIT is not enabled, LOCK won't enable
  // it.
  if (type_ == Type::SEM_ASYNC_LOCK && o->type_ == Type::SEM_WAIT)
    return false;

  // UNLOCK indep UNLOCK: ordering of two pop_front has no impact
  if (type_ == Type::SEM_UNLOCK && o->type_ == Type::SEM_UNLOCK)
    return false;

  // WAIT indep WAIT:
  // if both enabled (may happen in the initial value is sufficient), the ordering has no impact on the result.
  // If only one enabled, the other won't be enabled by the first one.
  // If none enabled, well, nothing will change.
  if (type_ == Type::SEM_WAIT && o->type_ == Type::SEM_WAIT)
    return false;

  if (o->type_ == Type::SEM_ASYNC_LOCK || o->type_ == Type::SEM_UNLOCK || o->type_ == Type::SEM_WAIT) {
    return sem_ == static_cast<const SemaphoreTransition*>(o)->sem_;
  }

  return false; // semaphores are INDEP with non-semaphore transitions
}

bool MutexTransition::reversible_race(const Transition* other) const
{
  if (other->type_ == Type::ACTOR_CREATE) {
    const ActorCreateTransition* ctransition = static_cast<const ActorCreateTransition*>(other);
    xbt_assert(aid_ == ctransition->get_child());
    return false;
  }

  switch (type_) {
    case Type::MUTEX_ASYNC_LOCK:
      return true; // MutexAsyncLock is always enabled
    case Type::MUTEX_TEST:
      return true; // MutexTest is always enabled
    case Type::MUTEX_TRYLOCK:
      return true; // MutexTrylock is always enabled
    case Type::MUTEX_UNLOCK:
      return true; // MutexUnlock is always enabled

    case Type::MUTEX_WAIT:
      // Only an Unlock or a CondVarAsynclock can be dependent with a Wait
      // and in this case, that transition enabled the wait
      // Not reversible
      return false;
    default:
      xbt_die("Unexpected transition type %s", to_c_str(type_));
  }
}

CondvarTransition::CondvarTransition(aid_t issuer, int times_considered, Type type, mc::Channel& channel)
    : Transition(type, issuer, times_considered)
{
  if (type == Type::CONDVAR_ASYNC_LOCK) {
    condvar_ = channel.unpack<unsigned>();
    mutex_   = channel.unpack<unsigned>();
  } else if (type == Type::CONDVAR_WAIT) {
    condvar_ = channel.unpack<unsigned>();
    mutex_   = channel.unpack<unsigned>();
    granted_ = channel.unpack<bool>();
    timeout_ = channel.unpack<bool>();
  } else if (type == Type::CONDVAR_SIGNAL || type == Type::CONDVAR_BROADCAST) {
    condvar_ = channel.unpack<unsigned>();
  } else
    xbt_die("type: %d %s", (int)type, to_c_str(type));
}

std::string CondvarTransition::to_string(bool verbose) const
{
  if (type_ == Type::CONDVAR_ASYNC_LOCK)
    return xbt::string_printf("%s(cond: %u, mutex: %u)", Transition::to_c_str(type_), condvar_, mutex_);
  if (type_ == Type::CONDVAR_SIGNAL || type_ == Type::CONDVAR_BROADCAST)
    return xbt::string_printf("%s(cond: %u)", Transition::to_c_str(type_), condvar_);
  if (type_ == Type::CONDVAR_WAIT)
    return xbt::string_printf("%s(cond: %u, mutex: %u, granted: %s, timeout: %s)", Transition::to_c_str(type_),
                              condvar_, mutex_, granted_ ? "yes" : "no", timeout_ ? "yes" : "none");
  THROW_IMPOSSIBLE;
}
bool CondvarTransition::depends(const Transition* o) const
{
  if (o->type_ < type_)
    return o->depends(this);

  // Actions executed by the same actor are always dependent
  if (o->aid_ == aid_)
    return true;

  // CondvarAsyncLock are dependent with wake up signals on the same condvar_
  if (type_ == Type::CONDVAR_ASYNC_LOCK && (o->type_ == Type::CONDVAR_SIGNAL || o->type_ == Type::CONDVAR_BROADCAST))
    return condvar_ == static_cast<const CondvarTransition*>(o)->condvar_;

  // Broadcast and Signal are dependent with wait since they can enable it
  if ((type_ == Type::CONDVAR_BROADCAST || type_ == Type::CONDVAR_SIGNAL) && o->type_ == Type::CONDVAR_WAIT)
    return condvar_ == static_cast<const CondvarTransition*>(o)->condvar_;

  // Wait is independent with itself

  // Independent with transitions that are neither Condvar nor Mutex related
  return false;
}
bool CondvarTransition::reversible_race(const Transition* other) const
{
  if (other->type_ == Type::ACTOR_CREATE) {
    const ActorCreateTransition* ctransition = static_cast<const ActorCreateTransition*>(other);
    xbt_assert(aid_ == ctransition->get_child());
    return false;
  }

  switch (type_) {
    case Type::CONDVAR_ASYNC_LOCK:
      switch (other->type_) {
        case Transition::Type::MUTEX_WAIT:
          return false;
        case Transition::Type::CONDVAR_BROADCAST:
        case Transition::Type::CONDVAR_SIGNAL:
          return true;
        default:
          xbt_die("Unexpected transition type %s was declared dependent with %s", to_c_str(type_),
                  to_c_str(other->type_));
      }

      // if wait can be executed in the first place, then broadcast and signal don't impact him
    case Type::CONDVAR_BROADCAST:
    case Type::CONDVAR_SIGNAL:
      xbt_assert(other->type_ == Transition::Type::CONDVAR_ASYNC_LOCK or
                 other->type_ == Transition::Type::CONDVAR_WAIT);
      return true;

      // broadcast and signal can enable the wait, hence this race is not always reversible
    case Type::CONDVAR_WAIT:
      xbt_assert(other->type_ == Transition::Type::CONDVAR_BROADCAST or
                 other->type_ == Transition::Type::CONDVAR_SIGNAL);
      return true;

    default:
      xbt_die("Unexpected transition type %s was declared dependent with %s", to_c_str(type_), to_c_str(other->type_));
  }
}

bool CondvarTransition::can_be_co_enabled(const Transition* o) const
{
  if (o->type_ < type_)
    return o->can_be_co_enabled(this);

  // Transition executed by the same actor can never be co-enabled
  if (o->aid_ == aid_)
    return false;

  // The only actions that can not be co-enabled are async lock asking for the same mutex
  if (type_ == Type::CONDVAR_ASYNC_LOCK && o->type_ == Type::CONDVAR_ASYNC_LOCK)
    return mutex_ != static_cast<const CondvarTransition*>(o)->condvar_;

  return true;
}

} // namespace simgrid::mc
