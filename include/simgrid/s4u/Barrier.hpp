/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

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

namespace simgrid::s4u {

/** @brief A classical barrier, but blocking in the simulation world.
 *
 * @beginrst
 * A Barrier allows a fixed number of actors to synchronize: each of them blocks in wait() until all of them reached
 * that point, after which they are all unblocked together. It is very similar to `std::barrier
 * <https://en.cppreference.com/w/cpp/thread/barrier>`_, but blocking the simulated actors instead of blocking real
 * threads.
 *
 * As for any S4U object, you can use the :ref:`RAII idiom <s4u_raii>` for memory management of Barriers. Use
 * :cpp:func:`create() <simgrid::s4u::Barrier::create()>` to get a :cpp:type:`simgrid::s4u::BarrierPtr` to a newly
 * created barrier, and only manipulate :cpp:type:`simgrid::s4u::BarrierPtr`.
 * @endrst
 */
class XBT_PUBLIC Barrier {
  kernel::activity::BarrierImpl* pimpl_;
  friend kernel::activity::BarrierImpl;

  explicit Barrier(kernel::activity::BarrierImpl* pimpl) : pimpl_(pimpl) {}

public:
#ifndef DOXYGEN
  Barrier(Barrier const&) = delete;
  Barrier& operator=(Barrier const&) = delete;
#endif

  /** \static Creates a barrier for the given amount of actors */
  static BarrierPtr create(unsigned int expected_actors);
  /** Blocks into the barrier. Every waiting actor is unblocked once the expected amount of actors reaches the barrier.
   *
   *  @return 0 for all actors but one: exactly one actor (picked arbitrarily) gets a non-zero value, so that it can be
   * elected to do some serial work before the others resume, if needed. */
  int wait();
  /** Returns some debug information about the barrier */
  std::string to_string() const;

#ifndef DOXYGEN
  /* refcounting */
  friend XBT_PUBLIC void intrusive_ptr_add_ref(Barrier* barrier);
  friend XBT_PUBLIC void intrusive_ptr_release(Barrier* barrier);
#endif
};
} // namespace simgrid::s4u

#endif
