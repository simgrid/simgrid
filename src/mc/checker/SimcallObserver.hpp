/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_OBSERVER_HPP
#define SIMGRID_MC_SIMCALL_OBSERVER_HPP

#include "simgrid/forward.h"

#include <string>

namespace simgrid {
namespace mc {

class SimcallObserver {
  kernel::actor::ActorImpl* const issuer_;

public:
  explicit SimcallObserver(kernel::actor::ActorImpl* issuer) : issuer_(issuer) {}
  kernel::actor::ActorImpl* get_issuer() const { return issuer_; }
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
  virtual void prepare(int times_considered) { /* Nothing to do by default */}

  /** Some simcalls may only be observable under some circumstances.
   * Most simcalls are not visible from the MC because they don't have an observer at all. */
  virtual bool is_visible() const { return true; }
  virtual std::string to_string(int times_considered) const = 0;
  virtual std::string dot_label() const                     = 0;
};

class RandomSimcall : public SimcallObserver {
  const int min_;
  const int max_;
  int next_value_ = 0;

public:
  RandomSimcall(smx_actor_t actor, int min, int max) : SimcallObserver(actor), min_(min), max_(max) {}
  int get_max_consider() const override;
  void prepare(int times_considered) override;
  std::string to_string(int times_considered) const override;
  std::string dot_label() const override;
  int get_value() const { return next_value_; }
};

class MutexUnlockSimcall : public SimcallObserver {
  using SimcallObserver::SimcallObserver;

public:
  std::string to_string(int times_considered) const override;
  std::string dot_label() const override;
};

class MutexLockSimcall : public SimcallObserver {
  kernel::activity::MutexImpl* const mutex_;
  const bool blocking_;

public:
  MutexLockSimcall(smx_actor_t actor, kernel::activity::MutexImpl* mutex, bool blocking = true)
      : SimcallObserver(actor), mutex_(mutex), blocking_(blocking)
  {
  }
  bool is_enabled() const override;
  std::string to_string(int times_considered) const override;
  std::string dot_label() const override;
  kernel::activity::MutexImpl* get_mutex() const { return mutex_; }
};
} // namespace mc
} // namespace simgrid

#endif
