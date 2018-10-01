/* Copyright (c) 2006-201. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.hpp"
#include "src/simix/smx_synchro_private.hpp"
#include "xbt/log.h"

#include "simgrid/s4u/Semaphore.hpp"

namespace simgrid {
namespace s4u {

Semaphore::Semaphore(unsigned int initial_capacity)
{
    sem_ = simgrid::simix::simcall([initial_capacity] { return SIMIX_sem_init(initial_capacity); });
}

Semaphore::~Semaphore()
{
    SIMIX_sem_destroy(sem_);
}

SemaphorePtr Semaphore::create(unsigned int initial_capacity)
{
    return SemaphorePtr(new Semaphore(initial_capacity));
}

void Semaphore::acquire()
{
    simcall_sem_acquire(sem_);
}

void Semaphore::release()
{
    simgrid::simix::simcall([this] { SIMIX_sem_release(sem_); });
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

}
}
