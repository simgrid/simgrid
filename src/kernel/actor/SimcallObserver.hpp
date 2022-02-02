/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_OBSERVER_HPP
#define SIMGRID_MC_SIMCALL_OBSERVER_HPP

#include "simgrid/forward.h"
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
  virtual bool is_enabled() const { return true; }

  /** Returns the amount of time that this transition can be used.
   *
   * If it's 1 (as with send/wait), there is no need to fork the state space exploration on this point.
   * If it's more than one (as with mc_random or waitany), we need to consider this transition several times to start
   * differing branches
   */
  virtual int get_max_consider() const { return 1; }

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

  /** We need to save the observer of simcalls as they get executed to later compute their dependencies in classical
   * DPOR */
  virtual SimcallObserver* clone() = 0;

  /** Computes the dependency relation */
  virtual bool depends(SimcallObserver* other);

  /** Some simcalls may only be observable under some conditions.
   * Most simcalls are not visible from the MC because they don't have an observer at all. */
  virtual bool is_visible() const { return true; }
  virtual std::string to_string(int times_considered) const = 0;
  virtual std::string dot_label(int times_considered) const = 0;
};

template <class T> class ResultingSimcall : public SimcallObserver {
  T result_;

public:
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
  SimcallObserver* clone() override
  {
    auto res         = new RandomSimcall(get_issuer(), min_, max_);
    res->next_value_ = next_value_;
    return res;
  }
  int get_max_consider() const override;
  void prepare(int times_considered) override;
  std::string to_string(int times_considered) const override;
  std::string dot_label(int times_considered) const override;
  int get_value() const { return next_value_; }
  bool depends(SimcallObserver* other) override;
};

class MutexSimcall : public SimcallObserver {
  activity::MutexImpl* const mutex_;

public:
  MutexSimcall(ActorImpl* actor, activity::MutexImpl* mutex) : SimcallObserver(actor), mutex_(mutex) {}
  activity::MutexImpl* get_mutex() const { return mutex_; }
  bool depends(SimcallObserver* other) override;
};

class MutexUnlockSimcall : public MutexSimcall {
  using MutexSimcall::MutexSimcall;

public:
  SimcallObserver* clone() override { return new MutexUnlockSimcall(get_issuer(), get_mutex()); }
  std::string to_string(int times_considered) const override;
  std::string dot_label(int times_considered) const override;
};

class MutexLockSimcall : public MutexSimcall {
  const bool blocking_;

public:
  MutexLockSimcall(ActorImpl* actor, activity::MutexImpl* mutex, bool blocking = true)
      : MutexSimcall(actor, mutex), blocking_(blocking)
  {
  }
  SimcallObserver* clone() override { return new MutexLockSimcall(get_issuer(), get_mutex(), blocking_); }
  bool is_enabled() const override;
  std::string to_string(int times_considered) const override;
  std::string dot_label(int times_considered) const override;
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
  SimcallObserver* clone() override { return new ConditionWaitSimcall(get_issuer(), cond_, mutex_, timeout_); }
  bool is_enabled() const override;
  bool is_visible() const override { return false; }
  std::string to_string(int times_considered) const override;
  std::string dot_label(int times_considered) const override;
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
  SimcallObserver* clone() override { return new SemAcquireSimcall(get_issuer(), sem_, timeout_); }
  bool is_enabled() const override;
  bool is_visible() const override { return false; }
  std::string to_string(int times_considered) const override;
  std::string dot_label(int times_considered) const override;
  activity::SemaphoreImpl* get_sem() const { return sem_; }
  double get_timeout() const { return timeout_; }
};

class ActivityTestSimcall : public ResultingSimcall<bool> {
  activity::ActivityImpl* const activity_;

public:
  ActivityTestSimcall(ActorImpl* actor, activity::ActivityImpl* activity)
      : ResultingSimcall(actor, true), activity_(activity)
  {
  }
  SimcallObserver* clone() override { return new ActivityTestSimcall(get_issuer(), activity_); }
  bool is_visible() const override { return true; }
  bool depends(SimcallObserver* other);
  std::string to_string(int times_considered) const override;
  std::string dot_label(int times_considered) const override;
  activity::ActivityImpl* get_activity() const { return activity_; }
};

class ActivityTestanySimcall : public ResultingSimcall<ssize_t> {
  const std::vector<activity::ActivityImpl*>& activities_;
  int next_value_ = 0;

public:
  ActivityTestanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities)
      : ResultingSimcall(actor, -1), activities_(activities)
  {
  }
  SimcallObserver* clone() override { return new ActivityTestanySimcall(get_issuer(), activities_); }
  bool is_visible() const override { return true; }
  int get_max_consider() const override;
  void prepare(int times_considered) override;
  std::string to_string(int times_considered) const override;
  std::string dot_label(int times_considered) const override;
  const std::vector<activity::ActivityImpl*>& get_activities() const { return activities_; }
  int get_value() const { return next_value_; }
};

class ActivityWaitSimcall : public ResultingSimcall<bool> {
  activity::ActivityImpl* activity_;
  const double timeout_;

public:
  ActivityWaitSimcall(ActorImpl* actor, activity::ActivityImpl* activity, double timeout)
      : ResultingSimcall(actor, false), activity_(activity), timeout_(timeout)
  {
  }
  SimcallObserver* clone() override { return new ActivityWaitSimcall(get_issuer(), activity_, timeout_); }
  bool is_visible() const override { return true; }
  bool is_enabled() const override;
  bool depends(SimcallObserver* other);
  std::string to_string(int times_considered) const override;
  std::string dot_label(int times_considered) const override;
  activity::ActivityImpl* get_activity() const { return activity_; }
  void set_activity(activity::ActivityImpl* activity) { activity_ = activity; }
  double get_timeout() const { return timeout_; }
};

class ActivityWaitanySimcall : public ResultingSimcall<ssize_t> {
  const std::vector<activity::ActivityImpl*>& activities_;
  const double timeout_;
  int next_value_ = 0;

public:
  ActivityWaitanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities, double timeout)
      : ResultingSimcall(actor, -1), activities_(activities), timeout_(timeout)
  {
  }
  SimcallObserver* clone() override { return new ActivityWaitanySimcall(get_issuer(), activities_, timeout_); }
  bool is_enabled() const override;
  bool is_visible() const override { return true; }
  void prepare(int times_considered) override;
  int get_max_consider() const override;
  std::string to_string(int times_considered) const override;
  std::string dot_label(int times_considered) const override;
  const std::vector<activity::ActivityImpl*>& get_activities() const { return activities_; }
  double get_timeout() const { return timeout_; }
  int get_value() const { return next_value_; }
};

} // namespace actor
} // namespace kernel
} // namespace simgrid

#endif
