/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_INSPECTOR_HPP
#define SIMGRID_MC_SIMCALL_INSPECTOR_HPP

#include "simgrid/forward.h"

#include <string>

namespace simgrid {
namespace mc {

class SimcallInspector {
  kernel::actor::ActorImpl* issuer_;

public:
  SimcallInspector(kernel::actor::ActorImpl* issuer) : issuer_(issuer) {}
  kernel::actor::ActorImpl* get_issuer() { return issuer_; }
  /** Whether this transition can currently be taken without blocking.
   *
   * For example, a mutex_lock is not enabled when the mutex is not free.
   * A comm_receive is not enabled before the corresponding send has been issued.
   */
  virtual bool is_enabled() { return true; }

  /** Check whether the simcall will still be firable on this execution path
   *
   * Return true if the simcall can be fired another time, and false if it gave all its content already.
   *
   * This is not to be mixed with is_enabled(). Even if is_pending() returns true,
   * is_enabled() may return false at a given point but will eventually return true further
   * on that execution path.
   *
   * If is_pending() returns false the first time, it means that the execution path is not branching
   * on that transition. Only one execution path goes out of that simcall.
   *
   * is_pending() is to decide whether we should branch a new execution path with this transition while
   * is_enabled() is about continuing the current execution path with that transition or saving it for later.
   *
   * This function should also do the choices that the platform would have done in non-MC settings.
   * For example if it's a waitany, pick the communication that should finish first.
   * If it's a random(), choose the next value to explore.
   */
  virtual bool is_pending(int times_considered) { return false; }

  /** Some simcalls may only be observable under some circumstances.
   * Most simcalls are not visible from the MC because they don't have an inspector at all. */
  virtual bool is_visible() { return true; }
  virtual std::string to_string(int times_considered);
  virtual std::string dot_label() = 0;
};

class RandomSimcall : public SimcallInspector {
  int min_;
  int max_;
  int next_value_ = 0;

public:
  RandomSimcall(smx_actor_t actor, int min, int max) : SimcallInspector(actor), min_(min), max_(max) {}
  bool is_pending(int times_considered) override;
  std::string to_string(int times_considered) override;
  std::string dot_label() override;
  int get_value();
};
} // namespace mc
} // namespace simgrid

#endif
