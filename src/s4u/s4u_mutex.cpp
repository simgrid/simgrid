/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.hpp"
#include "src/simix/smx_synchro_private.hpp"
#include "xbt/log.h"

#include "simgrid/s4u/Mutex.hpp"

namespace simgrid {
namespace s4u {

/** @brief Blocks the calling actor until the mutex can be obtained */
void Mutex::lock()
{
  simcall_mutex_lock(mutex_);
}

/** @brief Release the ownership of the mutex, unleashing a blocked actor (if any)
 *
 * Will fail if the calling actor does not own the mutex.
 */
void Mutex::unlock()
{
  simcall_mutex_unlock(mutex_);
}

/** @brief Acquire the mutex if it's free, and return false (without blocking) if not */
bool Mutex::try_lock()
{
  return simcall_mutex_trylock(mutex_);
}

/** @brief Create a new mutex
 *
 * See @ref s4u_raii.
 */
MutexPtr Mutex::createMutex()
{
  smx_mutex_t mutex = simcall_mutex_init();
  return MutexPtr(&mutex->mutex(), false);
}

}
}
