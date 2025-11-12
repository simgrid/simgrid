/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionSynchro.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/transition/TransitionActor.hpp"
#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "xbt/asserts.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/string.hpp"

#include <algorithm>
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
bool BarrierTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                        EventHandle other_handle) const
{
  if (other->type_ == Type::ACTOR_JOIN) {
    const ActorJoinTransition* ctransition = static_cast<const ActorJoinTransition*>(other);
    xbt_assert(aid_ == ctransition->get_target());
    return false;
  }

  switch (other->type_) {
    case Type::BARRIER_ASYNC_LOCK:
      return true; // BarrierAsyncLock is always enabled
    case Type::BARRIER_WAIT:
      // If the other event is a barrier lock event, then we are not reversible;
      // otherwise we are reversible.
      return type_ != Transition::Type::BARRIER_ASYNC_LOCK;
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

static bool is_sem_wait_fireable_without_unlock(const odpor::Execution* exec, EventHandle unlock_handle,
                                                EventHandle wait_handle)
{
  XBT_DEBUG(
      "Expertise has been required in the following matter:%s\nIs the wait <%s> enabled if we remove the unlock <%s>",
      exec->get_one_string_textual_trace().c_str(), exec->get_transition_for_handle(wait_handle)->to_string().c_str(),
      exec->get_transition_for_handle(unlock_handle)->to_string().c_str());

  unsigned sem_id  = static_cast<const SemaphoreTransition*>(exec->get_transition_for_handle(unlock_handle))->get_sem();
  int max_capacity = -1;
  int nb_lock      = 0;
  int nb_unlock    = 0;

  bool has_lock_been_found = false;

  for (EventHandle e = wait_handle; e <= wait_handle; e--) {

    if (exec->get_transition_for_handle(e)->type_ != Transition::Type::SEM_ASYNC_LOCK and
        exec->get_transition_for_handle(e)->type_ != Transition::Type::SEM_UNLOCK)
      continue;

    // We ignore lock that have occured after the lock corresponding to the considered wait
    if (not has_lock_been_found and exec->get_transition_for_handle(e)->type_ == Transition::Type::SEM_ASYNC_LOCK) {
      if (exec->get_actor_with_handle(e) != exec->get_actor_with_handle(wait_handle))
        continue;
      else
        has_lock_been_found = true;
    }

    auto sem_transition = static_cast<const SemaphoreTransition*>(exec->get_transition_for_handle(e));

    if (sem_transition->get_sem() != sem_id)
      continue;

    max_capacity = sem_transition->get_capacity();
    max_capacity += sem_transition->type_ == Transition::Type::SEM_ASYNC_LOCK ? 1 : -1;

    if (e == unlock_handle)
      continue;

    if (sem_transition->type_ == Transition::Type::SEM_ASYNC_LOCK)
      nb_lock++;
    if (sem_transition->type_ == Transition::Type::SEM_UNLOCK)
      nb_unlock++;
  }

  XBT_DEBUG("Conclusion is the following:\n\t[Max_capacity = %d]\n\t[nb_lock = %d]\n\t[nb_unlock = %d]", max_capacity,
            nb_lock, nb_unlock);
  XBT_DEBUG("Hence, the wait is %s without the unlock",
            max_capacity - nb_lock + nb_unlock >= 0 ? "fireable" : "not fireable");
  return max_capacity - nb_lock + nb_unlock >= 0;
}

bool SemaphoreTransition::reversible_race(const Transition* other, const odpor::Execution* exec,
                                          EventHandle this_handle, EventHandle other_handle) const
{
  if (other->type_ == Type::ACTOR_JOIN) {
    const ActorJoinTransition* jtransition = static_cast<const ActorJoinTransition*>(other);
    xbt_assert(aid_ == jtransition->get_target());
    return false;
  }

  switch (other->type_) {
    case Type::SEM_ASYNC_LOCK:
      return true; // SemAsyncLock is always enabled
    case Type::SEM_UNLOCK:
      return true; // SemUnlock is always enabled
    case Type::SEM_WAIT:
      xbt_assert(this->type_ == Type::SEM_UNLOCK);
      // Some times the race is not reversible
      return is_sem_wait_fireable_without_unlock(exec, this_handle, other_handle);
    default:
      xbt_die("Unexpected transition type %s", to_c_str(type_));
  }
}

std::string SemaphoreTransition::to_string(bool verbose) const
{
  if (type_ == Type::SEM_ASYNC_LOCK || type_ == Type::SEM_UNLOCK)
    return xbt::string_printf("%s(semaphore: %u, capacity: %d)", Transition::to_c_str(type_), sem_, capacity_);
  if (type_ == Type::SEM_WAIT)
    return xbt::string_printf("%s(semaphore: %u, capacity: %d, granted: %s)", Transition::to_c_str(type_), sem_,
                              capacity_, granted_ ? "yes" : "no");
  THROW_IMPOSSIBLE;
}
SemaphoreTransition::SemaphoreTransition(aid_t issuer, int times_considered, Type type, mc::Channel& channel)
    : Transition(type, issuer, times_considered)
{
  sem_      = channel.unpack<unsigned>();
  granted_  = channel.unpack<bool>();
  capacity_ = channel.unpack<int>();
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

bool MutexTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                      EventHandle other_handle) const
{
  if (other->type_ == Type::ACTOR_JOIN) {
    const ActorJoinTransition* jtransition = static_cast<const ActorJoinTransition*>(other);
    xbt_assert(aid_ == jtransition->get_target());
    return false;
  }

  switch (other->type_) {
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

static bool is_cv_wait_fireable_without_transition(const odpor::Execution* exec, EventHandle cv_wait_handle,
                                                   EventHandle other_handle)
{
  unsigned cv_id =
      static_cast<const CondvarTransition*>(exec->get_transition_for_handle(cv_wait_handle))->get_condvar();
  int signal_after_lock = 0;

  EventHandle lock_handle = 0;

  for (EventHandle e = cv_wait_handle - 1; e <= cv_wait_handle; e--) {

    if (e == other_handle)
      continue;

    if (const auto type = exec->get_transition_for_handle(e)->type_; type != Transition::Type::CONDVAR_ASYNC_LOCK and
                                                                     type != Transition::Type::CONDVAR_BROADCAST and
                                                                     type != Transition::Type::CONDVAR_SIGNAL)
      continue;

    auto cv_transition = static_cast<const CondvarTransition*>(exec->get_transition_for_handle(e));

    if (cv_transition->get_condvar() != cv_id)
      continue;

    if (cv_transition->aid_ == exec->get_actor_with_handle(cv_wait_handle)) {
      xbt_assert(cv_transition->type_ == Transition::Type::CONDVAR_ASYNC_LOCK,
                 "A condvar wait is always preceeded by an async_lock right? (aid: %ld)", cv_transition->aid_);
      lock_handle = e;
      break;
    }

    switch (cv_transition->type_) {
      case Transition::Type::CONDVAR_ASYNC_LOCK:
        break; // The other lock happening after the concerned one don't matter
      case Transition::Type::CONDVAR_BROADCAST:
        return true; // If there's still a broadcast after the asynclock and before the wait, the wait is fireable
      case Transition::Type::CONDVAR_SIGNAL:
        signal_after_lock++;
        break;
      default:
        xbt_die("What? What is this kind of transition? (%s) Fix Me!", Transition::to_c_str(cv_transition->type_));
    }
  }

  if (signal_after_lock == 0)
    return false; // If no signal happened after the lock, the wait can't be enabled

  // For the remaining cases, we have to find how many people are waiting on the cv before the lock is issued

  xbt_assert(lock_handle > 0);
  int currently_waiting_on_cv = 0;
  for (EventHandle e = 0; e < lock_handle; e++) {

    if (e == other_handle)
      continue;

    if (exec->get_transition_for_handle(e)->type_ != Transition::Type::CONDVAR_ASYNC_LOCK and
        exec->get_transition_for_handle(e)->type_ != Transition::Type::CONDVAR_BROADCAST and
        exec->get_transition_for_handle(e)->type_ != Transition::Type::CONDVAR_SIGNAL)
      continue;

    auto cv_transition = static_cast<const CondvarTransition*>(exec->get_transition_for_handle(e));

    if (cv_transition->get_condvar() != cv_id)
      continue;

    switch (cv_transition->type_) {
      case Transition::Type::CONDVAR_ASYNC_LOCK:
        currently_waiting_on_cv++;
        break;
      case Transition::Type::CONDVAR_BROADCAST:
        currently_waiting_on_cv = 0;
        break;
      case Transition::Type::CONDVAR_SIGNAL:
        // If no one is currently waiting, the signal is lost
        currently_waiting_on_cv = std::max(0, currently_waiting_on_cv - 1);
        break;
      default:
        xbt_die("What? What is this kind of transition? (%s) Fix Me!", Transition::to_c_str(cv_transition->type_));
    }
  }

  // For the wait to be enabled, we need to signal each actor currently waiting + the one we just put there
  return signal_after_lock > currently_waiting_on_cv;
}

bool CondvarTransition::reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                                        EventHandle other_handle) const
{
  if (other->type_ == Type::ACTOR_JOIN) {
    const ActorJoinTransition* jtransition = static_cast<const ActorJoinTransition*>(other);
    xbt_assert(aid_ == jtransition->get_target());
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

      // broadcast and signal can enable the wait, hence this race is not always reversible
    case Type::CONDVAR_BROADCAST:
    case Type::CONDVAR_SIGNAL:
      xbt_assert(other->type_ == Transition::Type::CONDVAR_ASYNC_LOCK or
                 other->type_ == Transition::Type::CONDVAR_WAIT);
      return is_cv_wait_fireable_without_transition(exec, other_handle, this_handle);

      // if wait can be executed in the first place, then broadcast and signal don't impact it
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
