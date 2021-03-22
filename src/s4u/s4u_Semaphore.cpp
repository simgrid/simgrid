/* Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.hpp"
#include "xbt/log.h"

#include "simgrid/forward.h"
#include "simgrid/s4u/Semaphore.hpp"
#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/mc/checker/SimcallObserver.hpp"

namespace simgrid {
namespace s4u {

SemaphorePtr Semaphore::create(unsigned int initial_capacity)
{
  auto* sem = new kernel::activity::SemaphoreImpl(initial_capacity);
  return SemaphorePtr(&sem->sem(), false);
}

void Semaphore::acquire()
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  mc::SemAcquireSimcall observer{issuer, pimpl_};
  kernel::actor::simcall_blocking<void>([&observer] { observer.get_sem()->acquire(observer.get_issuer(), -1.0); },
                                        &observer);
}

bool Semaphore::acquire_timeout(double timeout)
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  mc::SemAcquireSimcall observer{issuer, pimpl_, timeout};
  kernel::actor::simcall_blocking<void>(
      [&observer] { observer.get_sem()->acquire(observer.get_issuer(), observer.get_timeout()); }, &observer);
  return observer.get_result();
}

void Semaphore::release()
{
  kernel::actor::simcall([this] { pimpl_->release(); });
}

int Semaphore::get_capacity() const
{
  return kernel::actor::simcall([this] { return pimpl_->get_capacity(); });
}

bool Semaphore::would_block() const
{
  return kernel::actor::simcall([this] { return pimpl_->would_block(); });
}

/* refcounting of the intrusive_ptr is delegated to the implementation object */
void intrusive_ptr_add_ref(const Semaphore* sem)
{
  xbt_assert(sem);
  sem->pimpl_->ref();
}

void intrusive_ptr_release(const Semaphore* sem)
{
  xbt_assert(sem);
  sem->pimpl_->unref();
}

} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */
/** @brief creates a semaphore object of the given initial capacity */
sg_sem_t sg_sem_init(int initial_value)
{
  return simgrid::s4u::Semaphore::create(initial_value).detach();
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
  intrusive_ptr_release(sem);
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
