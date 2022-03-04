/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionSynchro.hpp"
#include "xbt/asserts.h"
#include "xbt/ex.h"
#include "xbt/string.hpp"

#include <inttypes.h>
#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_synchro, mc_transition, "Logging specific to MC synchronization transitions");

namespace simgrid {
namespace mc {

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

  if (auto* other = dynamic_cast<const BarrierTransition*>(o)) {
    if (bar_ != other->bar_)
      return false;

    // LOCK indep LOCK: requests are not ordered in a barrier
    if (type_ == Type::BARRIER_LOCK && other->type_ == Type::BARRIER_LOCK)
      return false;

    // WAIT indep WAIT: requests are not ordered
    if (type_ == Type::BARRIER_WAIT && other->type_ == Type::BARRIER_WAIT)
      return false;

    return true; // LOCK/WAIT is dependent because lock may enable wait
  }

  return false; // barriers are INDEP with non-barrier transitions
}

std::string MutexTransition::to_string(bool verbose) const
{
  return xbt::string_printf("%s(mutex: %" PRIxPTR ", owner:%ld)", Transition::to_c_str(type_), mutex_, owner_);
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

  // type_ <= other->type_ in  MUTEX_LOCK, MUTEX_TEST, MUTEX_TRYLOCK, MUTEX_UNLOCK, MUTEX_WAIT,

  if (auto* other = dynamic_cast<const MutexTransition*>(o)) {
    // Theorem 4.4.7: Any pair of synchronization actions of distinct actors concerning distinct mutexes are independent
    if (mutex_ != other->mutex_)
      return false;

    // Theorem 4.4.11: LOCK indep TEST/WAIT.
    //  If both enabled, the result does not depend on their order. If WAIT is not enabled, LOCK won't enable it.
    if (type_ == Type::MUTEX_LOCK && (other->type_ == Type::MUTEX_TEST || other->type_ == Type::MUTEX_WAIT))
      return false;

    // Theorem 4.4.8: LOCK indep UNLOCK.
    //  pop_front and push_back are independent.
    if (type_ == Type::MUTEX_LOCK && other->type_ == Type::MUTEX_UNLOCK)
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

std::string SemaphoreTransition::to_string(bool verbose) const
{
  if (type_ == Type::SEM_LOCK || type_ == Type::SEM_UNLOCK)
    return xbt::string_printf("%s(semaphore: %" PRIxPTR ")", Transition::to_c_str(type_), sem_);
  if (type_ == Type::SEM_WAIT)
    return xbt::string_printf("%s(semaphore: %" PRIxPTR ", granted: %s)", Transition::to_c_str(type_), sem_,
                              granted_ ? "yes" : "no");
  THROW_IMPOSSIBLE;
}
SemaphoreTransition::SemaphoreTransition(aid_t issuer, int times_considered, Type type, std::stringstream& stream)
    : Transition(type, issuer, times_considered)
{
  xbt_assert(stream >> sem_ >> granted_);
}
bool SemaphoreTransition::depends(const Transition* o) const
{
  if (o->type_ < type_)
    return o->depends(this);

  if (auto* other = dynamic_cast<const SemaphoreTransition*>(o)) {
    if (sem_ != other->sem_)
      return false;

    // LOCK indep UNLOCK: pop_front and push_back are independent.
    if (type_ == Type::SEM_LOCK && other->type_ == Type::SEM_UNLOCK)
      return false;

    // LOCK indep WAIT: If both enabled, ordering has no impact on the result. If WAIT is not enabled, LOCK won't enable
    // it.
    if (type_ == Type::SEM_LOCK && other->type_ == Type::SEM_WAIT)
      return false;

    // UNLOCK indep UNLOCK: ordering of two pop_front has no impact
    if (type_ == Type::SEM_UNLOCK && other->type_ == Type::SEM_UNLOCK)
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

} // namespace mc
} // namespace simgrid
