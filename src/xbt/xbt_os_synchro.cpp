/* Classical synchro schema, implemented on top of SimGrid                  */

/* Copyright (c) 2007-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.hpp"
#include "xbt/synchro.h"

#include "simgrid/simix.h" /* used implementation */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_sync, xbt, "Synchronization mechanism");

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
  SIMIX_mutex_unref((smx_mutex_t)mutex);
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
  try {
    simcall_cond_wait_timeout((smx_cond_t)cond, (smx_mutex_t)mutex, delay);
  } catch (xbt_ex& e) {
    if (e.category == timeout_error) {
      return 1;
    } else {
      throw; // rethrow the exceptions that I don't know
    }
  }
  return 0;
}

void xbt_cond_signal(xbt_cond_t cond)
{
  simcall_cond_signal((smx_cond_t)cond);
}

void xbt_cond_broadcast(xbt_cond_t cond)
{
  simcall_cond_broadcast((smx_cond_t)cond);
}

void xbt_cond_destroy(xbt_cond_t cond)
{
  SIMIX_cond_unref((smx_cond_t)cond);
}
