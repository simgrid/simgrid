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
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  int get_capacity() const { return capacity_; }
  unsigned int get_sem() const { return sem_; }
};

} // namespace simgrid::mc

#endif
