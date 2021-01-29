/* Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.hpp"
#include "xbt/log.h"

#include "simgrid/forward.h"
#include "simgrid/s4u/Semaphore.hpp"
#include "src/kernel/activity/SemaphoreImpl.hpp"

namespace simgrid {
namespace s4u {

Semaphore::Semaphore(unsigned int initial_capacity) : pimpl_(new kernel::activity::SemaphoreImpl(initial_capacity)) {}

Semaphore::~Semaphore()
{
  xbt_assert(not pimpl_->is_used(), "Cannot destroy semaphore since someone is still using it");
  delete pimpl_;
}

SemaphorePtr Semaphore::create(unsigned int initial_capacity)
{
  return SemaphorePtr(new Semaphore(initial_capacity));
}

void Semaphore::acquire()
{
  simcall_sem_acquire(pimpl_);
}

int Semaphore::acquire_timeout(double timeout)
{
  return simcall_sem_acquire_timeout(pimpl_, timeout);
}

void Semaphore::release()
{
  kernel::actor::simcall([this] { pimpl_->release(); });
}

int Semaphore::get_capacity() const
{
  return kernel::actor::simcall([this] { return pimpl_->get_capacity(); });
}

int Semaphore::would_block() const
{
  return kernel::actor::simcall([this] { return pimpl_->would_block(); });
}

void intrusive_ptr_add_ref(Semaphore* sem)
{
  xbt_assert(sem);
  sem->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(Semaphore* sem)
{
  xbt_assert(sem);
  if (sem->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete sem;
  }
}

} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */
/** @brief creates a semaphore object of the given initial capacity */
sg_sem_t sg_sem_init(int initial_value)
{
  return new simgrid::s4u::Semaphore(initial_value);
}

/** @brief locks on a semaphore object */
void sg_sem_acquire(sg_sem_t sem)
{
  sem->acquire();
}

/** @brief locks on a semaphore object up until the provided timeout expires */
int sg_sem_acquire_timeout(sg_sem_t sem, double timeout)
{
  return sem->acquire_timeout(timeout);
}

/** @brief releases the semaphore object */
void sg_sem_release(sg_sem_t sem)
{
  sem->release();
}

int sg_sem_get_capacity(const_sg_sem_t sem)
{
  return sem->get_capacity();
}

void sg_sem_destroy(const_sg_sem_t sem)
{
  delete sem;
}

/** @brief returns a boolean indicating if this semaphore would block at this very specific time
 *
 * Note that the returned value may be wrong right after the function call, when you try to use it...
 * But that's a classical semaphore issue, and SimGrid's semaphore are not different to usual ones here.
 */
int sg_sem_would_block(const_sg_sem_t sem)
{
  return sem->would_block();
}
