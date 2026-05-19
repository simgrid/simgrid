/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_SYNCHRO_HPP
#define SIMGRID_MC_TRANSITION_SYNCHRO_HPP

#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/remote/Channel.hpp"
#include "src/mc/transition/Transition.hpp"

#include <cstdint>

namespace simgrid::mc {

class BarrierTransition : public Transition {
  unsigned bar_;

public:
  std::string to_string(bool verbose) const override;
  BarrierTransition(Aid issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* o) const override
  {
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
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;
};

class CondvarTransition : public Transition {
  unsigned int condvar_;
  unsigned int mutex_;
  bool granted_;
  bool timeout_;

public:
  std::string to_string(bool verbose) const override;
  CondvarTransition(Aid issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* o) const override
  {

    // CondvarAsyncLock are dependent with wake up signals on the same condvar_
    if (type_ == Type::CONDVAR_ASYNC_LOCK && (o->type_ == Type::CONDVAR_SIGNAL || o->type_ == Type::CONDVAR_BROADCAST))
      return condvar_ == static_cast<const CondvarTransition*>(o)->condvar_;

    // Broadcast and Signal are dependent with wait since they can enable it
    if ((type_ == Type::CONDVAR_BROADCAST || type_ == Type::CONDVAR_SIGNAL) && o->type_ == Type::CONDVAR_WAIT)
      return condvar_ == static_cast<const CondvarTransition*>(o)->condvar_;

    // Wait is dependent with itself because it inserts a mutex_async_lock
    if (type_ == Type::CONDVAR_WAIT && o->type_ == Type::CONDVAR_WAIT)
      return mutex_ == static_cast<const CondvarTransition*>(o)->mutex_;

    // Independent with transitions that are neither Condvar nor Mutex related
    return false;
  }

  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  unsigned int get_condvar() const { return this->condvar_; }
  unsigned int get_mutex() const { return this->mutex_; }
  bool is_granted() const { return this->granted_; }
};

class MutexTransition : public Transition {
  uintptr_t mutex_;
  Aid owner_;

public:
  std::string to_string(bool verbose) const override;
  MutexTransition(Aid issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* o) const override
  {
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

    // A condvar_async_lock is behaving as a mutex_unlock, so it must have the same behavior regarding Mutex Wait
    if (o->type_ == Type::CONDVAR_ASYNC_LOCK) {
      if (type_ == Type::MUTEX_WAIT || type_ == Type::MUTEX_UNLOCK || type_ == Type::MUTEX_TRYLOCK)
        return mutex_ == static_cast<const CondvarTransition*>(o)->get_mutex();
    }
    // A condvar_wait is behaving as a mutex_async_lock
    if (o->type_ == Type::CONDVAR_WAIT) {
      if (type_ == Type::MUTEX_ASYNC_LOCK || type_ == Type::MUTEX_TRYLOCK)
        return mutex_ == static_cast<const CondvarTransition*>(o)->get_mutex();
    }

    // Theorem 4.4.7: Any pair of synchronization actions of distinct actors concerning distinct mutexes are independent
    // Since it's the last rule in this file, we can use the contrapositive version of the theorem
    if (o->type_ == Type::MUTEX_ASYNC_LOCK || o->type_ == Type::MUTEX_TEST || o->type_ == Type::MUTEX_TRYLOCK ||
        o->type_ == Type::MUTEX_UNLOCK || o->type_ == Type::MUTEX_WAIT)
      return (mutex_ == static_cast<const MutexTransition*>(o)->mutex_);

    return false;
  }

  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  uintptr_t get_mutex() const { return this->mutex_; }
  Aid get_owner() const { return this->owner_; }
};

class SemaphoreTransition : public Transition {
  unsigned int sem_; // ID
  bool granted_;
  int capacity_;

public:
  std::string to_string(bool verbose) const override;
  SemaphoreTransition(Aid issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* o) const override
  {
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

  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  int get_capacity() const { return capacity_; }
  unsigned int get_sem() const { return sem_; }
};

} // namespace simgrid::mc

#endif
