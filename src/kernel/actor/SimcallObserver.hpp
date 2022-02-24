/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_OBSERVER_HPP
#define SIMGRID_MC_SIMCALL_OBSERVER_HPP

#include "simgrid/forward.h"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"

#include <string>

namespace simgrid {
namespace kernel {
namespace actor {

class SimcallObserver {
  ActorImpl* const issuer_;

public:
  explicit SimcallObserver(ActorImpl* issuer) : issuer_(issuer) {}
  ActorImpl* get_issuer() const { return issuer_; }
  /** Whether this transition can currently be taken without blocking.
   *
   * For example, a mutex_lock is not enabled when the mutex is not free.
   * A comm_receive is not enabled before the corresponding send has been issued.
   */
  virtual bool is_enabled() { return true; }

  /** Returns the amount of time that this transition can be used.
   *
   * If it's 0, the transition is not enabled.
   * If it's 1 (as with send/wait), there is no need to fork the state space exploration on this point.
   * If it's more than one (as with mc_random or waitany), we need to consider this transition several times to start
   * differing branches
   */
  virtual int get_max_consider() { return 1; }

  /** Prepares the simcall to be used.
   *
   * For most simcalls, this does nothing. Once enabled, there is nothing to do to prepare a send().
   *
   * It is useful only for the simcalls that can be used several times, such as waitany() or random().
   * For them, prepare() selects the right outcome for the time being considered.
   *
   * The first time a simcall is considered, times_considered is 0, not 1.
   */
  virtual void prepare(int times_considered)
  { /* Nothing to do by default */
  }

  /** Serialize to the given string buffer */
  virtual void serialize(std::stringstream& stream) const;

  /** Some simcalls may only be observable under some conditions.
   * Most simcalls are not visible from the MC because they don't have an observer at all. */
  virtual bool is_visible() const { return true; }
};

template <class T> class ResultingSimcall : public SimcallObserver {
  T result_;

public:
  ResultingSimcall() = default;
  ResultingSimcall(ActorImpl* actor, T default_result) : SimcallObserver(actor), result_(default_result) {}
  void set_result(T res) { result_ = res; }
  T get_result() const { return result_; }
};

class RandomSimcall : public SimcallObserver {
  const int min_;
  const int max_;
  int next_value_ = 0;

public:
  RandomSimcall(ActorImpl* actor, int min, int max) : SimcallObserver(actor), min_(min), max_(max)
  {
    xbt_assert(min < max);
  }
  void serialize(std::stringstream& stream) const override;
  int get_max_consider() override;
  void prepare(int times_considered) override;
  int get_value() const { return next_value_; }
};

class ConditionWaitSimcall : public ResultingSimcall<bool> {
  activity::ConditionVariableImpl* const cond_;
  activity::MutexImpl* const mutex_;
  const double timeout_;

public:
  ConditionWaitSimcall(ActorImpl* actor, activity::ConditionVariableImpl* cond, activity::MutexImpl* mutex,
                       double timeout = -1.0)
      : ResultingSimcall(actor, false), cond_(cond), mutex_(mutex), timeout_(timeout)
  {
  }
  bool is_enabled() override;
  bool is_visible() const override { return false; }
  activity::ConditionVariableImpl* get_cond() const { return cond_; }
  activity::MutexImpl* get_mutex() const { return mutex_; }
  double get_timeout() const { return timeout_; }
};

class SemAcquireSimcall : public ResultingSimcall<bool> {
  activity::SemaphoreImpl* const sem_;
  const double timeout_;

public:
  SemAcquireSimcall(ActorImpl* actor, activity::SemaphoreImpl* sem, double timeout = -1.0)
      : ResultingSimcall(actor, false), sem_(sem), timeout_(timeout)
  {
  }
  bool is_enabled() override;
  bool is_visible() const override { return false; }
  activity::SemaphoreImpl* get_sem() const { return sem_; }
  double get_timeout() const { return timeout_; }
};

} // namespace actor
} // namespace kernel
} // namespace simgrid

#endif
