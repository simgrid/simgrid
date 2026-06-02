/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_SYNCHRO_HPP
#define SIMGRID_MC_TRANSITION_SYNCHRO_HPP

#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/remote/Channel.hpp"
#include "src/mc/transition/Transition.hpp"

#include <cstdint>
#include <stdexcept>

namespace simgrid::mc {

class BarrierTransition final : public Transition {
  unsigned bar_;

public:
  std::string to_string(bool verbose) const override;
  BarrierTransition(Aid issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* o) const override
  {
    if (o->type_ != Type::BARRIER_ASYNC_LOCK && o->type_ != Type::BARRIER_WAIT)
      return false; // barriers are INDEP with non-barrier transitions

    const auto* other = static_cast<const BarrierTransition*>(o);
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
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;
};

class CondvarTransition final : public Transition {
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

class MutexTransition final : public Transition {
  uintptr_t mutex_;
  Aid owner_;

public:
  std::string to_string(bool verbose) const override;
  MutexTransition(Aid issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* o) const override
  {
    // type_ <= other->type_ in  MUTEX_LOCK, MUTEX_TEST, MUTEX_TRYLOCK, MUTEX_UNLOCK, MUTEX_WAIT,
    // xbt_assert(type_ <= o->type_);

    // Theorem 4.4.7: Any pair of synchronization actions of distinct actors concerning distinct mutexes are independent
    if (mutex_ != static_cast<const MutexTransition*>(o)->mutex_)
      return false;

    // This is the same mutex and we have only mutex transitions. Use a constexpr look-up table so that the code remains
    // small enough to be inlined
    enum MIdx : int { LOCK = 0, TEST = 1, TRYLOCK = 2, UNLOCK = 3, WAIT = 4, N = 5 };

    static constexpr auto to_idx = [](Type t) constexpr noexcept -> int {
      switch (t) {
        case Type::MUTEX_ASYNC_LOCK:
          return LOCK;
        case Type::MUTEX_TEST:
          return TEST;
        case Type::MUTEX_TRYLOCK:
          return TRYLOCK;
        case Type::MUTEX_UNLOCK:
          return UNLOCK;
        case Type::MUTEX_WAIT:
          return WAIT;
        default:
          return -1;
      }
    };

    static constexpr auto dep_table = []() consteval {
      std::array<std::array<int8_t, N>, N> dep{};
      for (auto& row : dep)
        row.fill(-1); // -1 = undefined, checked below

      auto indep  = [&](MIdx a, MIdx b) { dep[a][b] = dep[b][a] = 0; };
      auto depend = [&](MIdx a, MIdx b) { dep[a][b] = dep[b][a] = 1; };

      // Th 4.4.11 : LOCK indep with TEST and WAIT
      indep(LOCK, TEST);
      indep(LOCK, WAIT);
      // Th 4.4.8 : LOCK indep UNLOCK (push_back and pop_front are independent)
      indep(LOCK, UNLOCK);
      // Th 4.4.9 : Any combination of WAIT and TEST are
      indep(TEST, TEST);
      indep(TEST, WAIT);
      indep(WAIT, WAIT);
      // TEST is a pure function: TRYLOCK always fail when TEST is actived
      indep(TEST, TRYLOCK);
      // TRYLOCK always fail when WAIT is actived
      indep(TRYLOCK, WAIT);
      // UNLOCK/UNLOCK: no more than one UNLOCK can be actived at a given point
      indep(UNLOCK, UNLOCK);

      // Any other cases are dependent
      depend(LOCK, LOCK);
      depend(LOCK, TRYLOCK);
      depend(TEST, UNLOCK);
      depend(TRYLOCK, TRYLOCK);
      depend(TRYLOCK, UNLOCK);
      depend(UNLOCK, WAIT);

      // Raise a compilation error if a cell is left uninitialized
      for (int i = 0; i < N; i++)
        for (int j = i; j < N; j++)
          if (dep[i][j] == -1)
            throw std::logic_error(std::format("Missing (in)dep relation: {} {}", i, j));

      return dep;
    }();

    return static_cast<bool>(dep_table[to_idx(type_)][to_idx(o->type_)]);
  }

  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  uintptr_t get_mutex() const { return this->mutex_; }
  Aid get_owner() const { return this->owner_; }
};

class SemaphoreTransition final : public Transition {
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
