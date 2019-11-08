/* Classical synchro schema, implemented on top of SimGrid                  */

/* Copyright (c) 2007-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/simix.h" /* used implementation */
#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "xbt/synchro.h"

/****** mutex related functions ******/
xbt_mutex_t xbt_mutex_init(void)
{
  return (xbt_mutex_t)simcall_mutex_init();
}

void xbt_mutex_acquire(xbt_mutex_t mutex)
{
  simcall_mutex_lock((smx_mutex_t)mutex);
}

int xbt_mutex_try_acquire(xbt_mutex_t mutex)
{
  return simcall_mutex_trylock((smx_mutex_t)mutex);
}

void xbt_mutex_release(xbt_mutex_t mutex)
{
  simcall_mutex_unlock((smx_mutex_t)mutex);
}

void xbt_mutex_destroy(xbt_mutex_t mutex)
{
  if (mutex != nullptr)
    mutex->unref();
}

/***** condition related functions *****/
xbt_cond_t xbt_cond_init(void)
{
  return (xbt_cond_t)simcall_cond_init();
}

void xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex)
{
  simcall_cond_wait((smx_cond_t)cond, (smx_mutex_t)mutex);
}

int xbt_cond_timedwait(xbt_cond_t cond, xbt_mutex_t mutex, double delay)
{
  return simcall_cond_wait_timeout((smx_cond_t)cond, (smx_mutex_t)mutex, delay);
}

void xbt_cond_signal(xbt_cond_t cond)
{
  cond->cond_.notify_one();
}

void xbt_cond_broadcast(xbt_cond_t cond)
{
  cond->cond_.notify_all();
}

void xbt_cond_destroy(xbt_cond_t cond)
{
  delete cond;
}
