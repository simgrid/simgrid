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
  BarrierTransition(aid_t issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;
};

class MutexTransition : public Transition {
  uintptr_t mutex_;
  aid_t owner_;

public:
  std::string to_string(bool verbose) const override;
  MutexTransition(aid_t issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* other) const override;
  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  uintptr_t get_mutex() const { return this->mutex_; }
  aid_t get_owner() const { return this->owner_; }
};

class SemaphoreTransition : public Transition {
  unsigned int sem_; // ID
  bool granted_;
  int capacity_;

public:
  std::string to_string(bool verbose) const override;
  SemaphoreTransition(aid_t issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  int get_capacity() const { return capacity_; }
  unsigned int get_sem() const { return sem_; }
};

class CondvarTransition : public Transition {
  unsigned int condvar_;
  unsigned int mutex_;
  bool granted_;
  bool timeout_;

public:
  std::string to_string(bool verbose) const override;
  CondvarTransition(aid_t issuer, int times_considered, Type type, mc::Channel& channel);
  bool depends(const Transition* other) const override;
  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  unsigned int get_condvar() const { return this->condvar_; }
  unsigned int get_mutex() const { return this->mutex_; }
  bool is_granted() const { return this->granted_; }
};

} // namespace simgrid::mc

#endif
