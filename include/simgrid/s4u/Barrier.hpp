/* Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_BARRIER_HPP
#define SIMGRID_S4U_BARRIER_HPP

#include <simgrid/barrier.h>
#include <simgrid/chrono.hpp>
#include <simgrid/forward.h>
#include <simgrid/s4u/ConditionVariable.hpp>
#include <simgrid/s4u/Mutex.hpp>

#include <atomic>
#include <future>

namespace simgrid {
namespace s4u {

class XBT_PUBLIC Barrier {
private:
  MutexPtr mutex_            = Mutex::create();
  ConditionVariablePtr cond_ = ConditionVariable::create();
  unsigned int expected_actors_;
  unsigned int arrived_actors_ = 0;

  /* refcounting */
  std::atomic_int_fast32_t refcount_{0};

public:
  /** Creates a barrier for the given amount of actors */
  explicit Barrier(unsigned int expected_actors) : expected_actors_(expected_actors) {}
#ifndef DOXYGEN
  Barrier(Barrier const&) = delete;
  Barrier& operator=(Barrier const&) = delete;
#endif

  /** Creates a barrier for the given amount of actors */
  static BarrierPtr create(unsigned int expected_actors);
  /** Blocks into the barrier. Every waiting actors will be unlocked once the expected amount of actors reaches the barrier */
  int wait();

#ifndef DOXYGEN
  /* refcounting */
  friend XBT_PUBLIC void intrusive_ptr_add_ref(Barrier* barrier);
  friend XBT_PUBLIC void intrusive_ptr_release(Barrier* barrier);
#endif
};
} // namespace s4u
} // namespace simgrid

#endif
