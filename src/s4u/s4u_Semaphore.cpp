/* Copyright (c) 2018-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/modelchecker.h>
#include <simgrid/s4u/Semaphore.hpp>
#include <simgrid/semaphore.h>

#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/kernel/actor/SynchroObserver.hpp"
#include "src/mc/mc_replay.hpp"

namespace simgrid {
namespace s4u {

SemaphorePtr Semaphore::create(unsigned int initial_capacity)
{
  auto* sem = new kernel::activity::SemaphoreImpl(initial_capacity);
  return SemaphorePtr(&sem->sem(), false);
}

void Semaphore::acquire()
{
  acquire_timeout(-1);
}

bool Semaphore::acquire_timeout(double timeout)
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();

  if (MC_is_active() || MC_record_replay_is_active()) { // Split in 2 simcalls for transition persistency
    kernel::actor::SemaphoreObserver lock_observer{issuer, mc::Transition::Type::SEM_LOCK, pimpl_};
    auto acquisition =
        kernel::actor::simcall_answered([issuer, this] { return pimpl_->acquire_async(issuer); }, &lock_observer);

    kernel::actor::SemaphoreAcquisitionObserver wait_observer{issuer, mc::Transition::Type::SEM_WAIT, acquisition.get(),
                                                              timeout};
    return kernel::actor::simcall_blocking(
        [issuer, acquisition, timeout] { return acquisition->wait_for(issuer, timeout); }, &wait_observer);

  } else { // Do it in one simcall only and without observer
    kernel::actor::SemaphoreAcquisitionObserver observer{issuer, mc::Transition::Type::SEM_WAIT, nullptr, timeout};
    return kernel::actor::simcall_blocking(
        [this, issuer, timeout] { return pimpl_->acquire_async(issuer)->wait_for(issuer, timeout); }, &observer);
  }
}

void Semaphore::release()
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::SemaphoreObserver observer{issuer, mc::Transition::Type::SEM_UNLOCK, pimpl_};

  kernel::actor::simcall_answered([this] { pimpl_->release(); }, &observer);
}

int Semaphore::get_capacity() const
{
  return pimpl_->get_capacity();
}

bool Semaphore::would_block() const
{
  return pimpl_->would_block();
}

/* refcounting of the intrusive_ptr is delegated to the implementation object */
void intrusive_ptr_add_ref(const Semaphore* sem)
{
  intrusive_ptr_add_ref(sem->pimpl_);
}

void intrusive_ptr_release(const Semaphore* sem)
{
  intrusive_ptr_release(sem->pimpl_);
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
