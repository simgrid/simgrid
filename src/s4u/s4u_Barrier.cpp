/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <exception>
#include <mutex>

#include "simgrid/Exception.hpp"
#include "simgrid/barrier.h"
#include "simgrid/s4u/Barrier.hpp"
#include "simgrid/simix.h"
#include "xbt/log.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_barrier, s4u, "S4U barrier");

namespace simgrid {
namespace s4u {

/** @brief Create a new barrier
 *
 * See @ref s4u_raii.
 */
BarrierPtr Barrier::create(unsigned int expected_actors)
{
  return BarrierPtr(new Barrier(expected_actors));
}

/** @brief Block the current actor until all expected actors reach the barrier.
 *
 * This method is meant to be somewhat consistent with the pthread_barrier_wait function.
 *
 * @return 0 for all actors but one: exactly one actor will get SG_BARRIER_SERIAL_THREAD as a return value.
 */
int Barrier::wait()
{
  mutex_->lock();
  arrived_actors_++;
  XBT_DEBUG("waiting %p %u/%u", this, arrived_actors_, expected_actors_);
  if (arrived_actors_ == expected_actors_) {
    cond_->notify_all();
    mutex_->unlock();
    arrived_actors_ = 0;
    return SG_BARRIER_SERIAL_THREAD;
  }

  cond_->wait(mutex_);
  mutex_->unlock();
  return 0;
}

void intrusive_ptr_add_ref(Barrier* barrier)
{
  xbt_assert(barrier);
  barrier->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(Barrier* barrier)
{
  xbt_assert(barrier);
  if (barrier->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete barrier;
  }
}
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

sg_bar_t sg_barrier_init(unsigned int count)
{
  return new simgrid::s4u::Barrier(count);
}

/** @brief Initializes a barrier, with count elements */
void sg_barrier_destroy(sg_bar_t bar)
{
  delete bar;
}

/** @brief Performs a barrier already initialized */
int sg_barrier_wait(sg_bar_t bar)
{
  return bar->wait();
}
