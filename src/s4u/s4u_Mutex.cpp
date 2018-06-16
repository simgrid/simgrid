/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Mutex.hpp"
#include "src/kernel/activity/MutexImpl.hpp"

namespace simgrid {
namespace s4u {

/** @brief Blocks the calling actor until the mutex can be obtained */
void Mutex::lock()
{
  simcall_mutex_lock(pimpl_);
}

/** @brief Release the ownership of the mutex, unleashing a blocked actor (if any)
 *
 * Will fail if the calling actor does not own the mutex.
 */
void Mutex::unlock()
{
  simcall_mutex_unlock(pimpl_);
}

/** @brief Acquire the mutex if it's free, and return false (without blocking) if not */
bool Mutex::try_lock()
{
  return simcall_mutex_trylock(pimpl_);
}

/** @brief Create a new mutex
 *
 * See @ref s4u_raii.
 */
MutexPtr Mutex::create()
{
  smx_mutex_t mutex = simcall_mutex_init();
  return MutexPtr(&mutex->mutex(), false);
}

/* refcounting of the intrusive_ptr is delegated to the implementation object */
void intrusive_ptr_add_ref(Mutex* mutex)
{
  xbt_assert(mutex);
  SIMIX_mutex_ref(mutex->pimpl_);
}
void intrusive_ptr_release(Mutex* mutex)
{
  xbt_assert(mutex);
  SIMIX_mutex_unref(mutex->pimpl_);
}
} // namespace s4u
} // namespace simgrid
