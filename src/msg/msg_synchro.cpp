/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.hpp"

#include "msg_private.hpp"
#include "src/simix/smx_private.hpp"
#include "xbt/synchro.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_synchro, msg, "Logging specific to MSG (synchro)");

/** @addtogroup msg_synchro
 *
 *  @{
 */

/** @brief creates a semaphore object of the given initial capacity */
msg_sem_t MSG_sem_init(int initial_value) {
  return simgrid::simix::kernelImmediate([initial_value] { return SIMIX_sem_init(initial_value); });
}

/** @brief locks on a semaphore object */
void MSG_sem_acquire(msg_sem_t sem) {
  simcall_sem_acquire(sem);
}

/** @brief locks on a semaphore object up until the provided timeout expires */
msg_error_t MSG_sem_acquire_timeout(msg_sem_t sem, double timeout) {
  msg_error_t res = MSG_OK;
  try {
    simcall_sem_acquire_timeout(sem,timeout);
  } catch(xbt_ex& e) {
    if (e.category == timeout_error)
      return MSG_TIMEOUT;
    throw;
  }
  return res;
}

/** @brief releases the semaphore object */
void MSG_sem_release(msg_sem_t sem) {
  simgrid::simix::kernelImmediate([sem] { SIMIX_sem_release(sem); });
}

int MSG_sem_get_capacity(msg_sem_t sem) {
  return simgrid::simix::kernelImmediate([sem] { return SIMIX_sem_get_capacity(sem); });
}

void MSG_sem_destroy(msg_sem_t sem) {
  SIMIX_sem_destroy(sem);
}

/** @brief returns a boolean indicating if this semaphore would block at this very specific time
 *
 * Note that the returned value may be wrong right after the function call, when you try to use it...
 * But that's a classical semaphore issue, and SimGrid's semaphore are not different to usual ones here.
 */
int MSG_sem_would_block(msg_sem_t sem) {
  return simgrid::simix::kernelImmediate([sem] { return SIMIX_sem_would_block(sem); });
}

/*-**** barrier related functions ****-*/
struct s_msg_bar_t {
  xbt_mutex_t mutex;
  xbt_cond_t cond;
  unsigned int arrived_processes;
  unsigned int expected_processes;
};

/** @brief Initializes a barrier, with count elements */
msg_bar_t MSG_barrier_init(unsigned int count) {
  msg_bar_t bar           = new s_msg_bar_t;
  bar->expected_processes = count;
  bar->arrived_processes  = 0;
  bar->mutex              = xbt_mutex_init();
  bar->cond               = xbt_cond_init();
  return bar;
}

/** @brief Initializes a barrier, with count elements */
void MSG_barrier_destroy(msg_bar_t bar) {
  xbt_mutex_destroy(bar->mutex);
  xbt_cond_destroy(bar->cond);
  delete bar;
}

/** @brief Performs a barrier already initialized */
int MSG_barrier_wait(msg_bar_t bar) {
  xbt_mutex_acquire(bar->mutex);
  bar->arrived_processes++;
  XBT_DEBUG("waiting %p %u/%u", bar, bar->arrived_processes, bar->expected_processes);
  if (bar->arrived_processes == bar->expected_processes) {
    xbt_cond_broadcast(bar->cond);
    xbt_mutex_release(bar->mutex);
    bar->arrived_processes = 0;
    return MSG_BARRIER_SERIAL_PROCESS;
  }

  xbt_cond_wait(bar->cond, bar->mutex);
  xbt_mutex_release(bar->mutex);
  return 0;
}
/**@}*/
