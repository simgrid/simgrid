/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionSynchro.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <inttypes.h>
#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_synchro, mc_transition, "Logging specific to MC synchronization transitions");

namespace simgrid {
namespace mc {
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
  }

  return true; // FIXME: TODO
}

} // namespace mc
} // namespace simgrid
