/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionSynchro.hpp"
#include "src/mc/mc_forward.hpp"
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
BarrierTransition::BarrierTransition(aid_t issuer, int times_considered, Type type, std::stringstream& stream)
    : Transition(type, issuer, times_considered)
{
  xbt_assert(stream >> bar_);
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

MutexTransition::MutexTransition(aid_t issuer, int times_considered, Type type, std::stringstream& stream)
    : Transition(type, issuer, times_considered)
{
  xbt_assert(stream >> mutex_ >> owner_);
}

bool MutexTransition::depends(const Transition* o) const
{
  if (o->type_ < type_)
    return o->depends(this);

  // Actions executed by the same actor are always dependent
  if (o->aid_ == aid_)
    return true;

  // type_ <= other->type_ in  MUTEX_LOCK, MUTEX_TEST, MUTEX_TRYLOCK, MUTEX_UNLOCK, MUTEX_WAIT,

  if (const auto* other = dynamic_cast<const MutexTransition*>(o)) {
    // Theorem 4.4.7: Any pair of synchronization actions of distinct actors concerning distinct mutexes are independent
    if (mutex_ != other->mutex_)
      return false;

    // Theorem 4.4.11: LOCK indep TEST/WAIT.
    //  If both enabled, the result does not depend on their order. If WAIT is not enabled, LOCK won't enable it.
    if (type_ == Type::MUTEX_ASYNC_LOCK && (other->type_ == Type::MUTEX_TEST || other->type_ == Type::MUTEX_WAIT))
      return false;

    // Theorem 4.4.8: LOCK indep UNLOCK.
    //  pop_front and push_back are independent.
    if (type_ == Type::MUTEX_ASYNC_LOCK && other->type_ == Type::MUTEX_UNLOCK)
      return false;

    // Theorem 4.4.9: LOCK indep UNLOCK.
    //  any combination of wait and test is indenpendent.
    if ((type_ == Type::MUTEX_WAIT || type_ == Type::MUTEX_TEST) &&
        (other->type_ == Type::MUTEX_WAIT || other->type_ == Type::MUTEX_TEST))
      return false;

    // TEST is a pure function; TEST/WAIT won't change the owner; TRYLOCK will always fail if TEST is enabled (because a
    // request is queued)
    if (type_ == Type::MUTEX_TEST &&
        (other->type_ == Type::MUTEX_TEST || other->type_ == Type::MUTEX_TRYLOCK || other->type_ == Type::MUTEX_WAIT))
      return false;

    // TRYLOCK will always fail if TEST is enabled (because a request is queued), and may not overpass the WAITed
    // request in the queue
    if (type_ == Type::MUTEX_TRYLOCK && other->type_ == Type::MUTEX_WAIT)
      return false;

    // FIXME: UNLOCK indep WAIT/TEST iff wait/test are not first in the waiting queue
    return true;
  }

  return false; // mutexes are INDEP with non-mutex transitions
}

bool SemaphoreTransition::reversible_race(const Transition* other) const
{
  switch (type_) {
    case Type::SEM_ASYNC_LOCK:
      return true; // SemAsyncLock is always enabled
    case Type::SEM_UNLOCK:
      return true; // SemUnlock is always enabled
    case Type::SEM_WAIT:
      if (other->type_ == Transition::Type::SEM_UNLOCK &&
          static_cast<const SemaphoreTransition*>(other)->get_capacity() <= 1) {
        return false;
      }
      xbt_die("SEM_WAIT that is dependent with a SEM_UNLOCK should not be reversible. FixMe");
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
SemaphoreTransition::SemaphoreTransition(aid_t issuer, int times_considered, Type type, std::stringstream& stream)
    : Transition(type, issuer, times_considered)
{
  xbt_assert(stream >> sem_ >> granted_ >> capacity_);
}
bool SemaphoreTransition::depends(const Transition* o) const
{
  if (o->type_ < type_)
    return o->depends(this);

  // Actions executed by the same actor are always dependent
  if (o->aid_ == aid_)
    return true;

  if (const auto* other = dynamic_cast<const SemaphoreTransition*>(o)) {
    if (sem_ != other->sem_)
      return false;

    // LOCK indep UNLOCK: pop_front and push_back are independent.
    if (type_ == Type::SEM_ASYNC_LOCK && other->type_ == Type::SEM_UNLOCK)
      return false;

    // LOCK indep WAIT: If both enabled, ordering has no impact on the result. If WAIT is not enabled, LOCK won't enable
    // it.
    if (type_ == Type::SEM_ASYNC_LOCK && other->type_ == Type::SEM_WAIT)
      return false;

    // UNLOCK indep UNLOCK: ordering of two pop_front has no impact
    if (type_ == Type::SEM_UNLOCK && other->type_ == Type::SEM_UNLOCK)
      return false;

    // UNLOCK indep with a WAIT if the semaphore had enought capacity anyway
    if (type_ == Type::SEM_UNLOCK && capacity_ > 1 && other->type_ == Type::SEM_WAIT)
      return false;

    // WAIT indep WAIT:
    // if both enabled (may happen in the initial value is sufficient), the ordering has no impact on the result.
    // If only one enabled, the other won't be enabled by the first one.
    // If none enabled, well, nothing will change.
    if (type_ == Type::SEM_WAIT && other->type_ == Type::SEM_WAIT)
      return false;

    return true; // Other semaphore cases are dependent
  }

  return false; // semaphores are INDEP with non-semaphore transitions
}

bool MutexTransition::reversible_race(const Transition* other) const
{
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
      // Only an Unlock can be dependent with a Wait
      // and in this case, that Unlock enabled the wait
      // Not reversible
      return false;
    default:
      xbt_die("Unexpected transition type %s", to_c_str(type_));
  }
}

} // namespace simgrid::mc
