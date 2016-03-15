/* xbt_synchro -- Synchronization virtualized depending on whether we are   */
/*                in simulation or real life (act on simulated processes)   */

/* This is the simulation implementation, using simix.                      */

/* Copyright (c) 2007-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/synchro_core.h"

#include "simgrid/simix.h"        /* used implementation */
#include "../simix/smx_private.h" /* FIXME */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_sync, xbt, "Synchronization mechanism");

/****** mutex related functions ******/
struct s_xbt_mutex_ {
  s_smx_mutex_t mutex;
};

xbt_mutex_t xbt_mutex_init(void)
{
  return (xbt_mutex_t) simcall_mutex_init();
}

void xbt_mutex_acquire(xbt_mutex_t mutex)
{
  simcall_mutex_lock((smx_mutex_t) mutex);
}

int xbt_mutex_try_acquire(xbt_mutex_t mutex)
{
  return simcall_mutex_trylock((smx_mutex_t) mutex);
}

void xbt_mutex_release(xbt_mutex_t mutex)
{
  simcall_mutex_unlock((smx_mutex_t) mutex);
}

void xbt_mutex_destroy(xbt_mutex_t mutex)
{
  SIMIX_mutex_destroy((smx_mutex_t) mutex);
}

/***** condition related functions *****/
struct s_xbt_cond_ {
  s_smx_cond_t cond;
};

xbt_cond_t xbt_cond_init(void)
{
  return (xbt_cond_t) simcall_cond_init();
}

void xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex)
{
  simcall_cond_wait((smx_cond_t) cond, (smx_mutex_t) mutex);
}

void xbt_cond_timedwait(xbt_cond_t cond, xbt_mutex_t mutex, double delay)
{
  simcall_cond_wait_timeout((smx_cond_t) cond, (smx_mutex_t) mutex, delay);
}

void xbt_cond_signal(xbt_cond_t cond)
{
  simcall_cond_signal((smx_cond_t) cond);
}

void xbt_cond_broadcast(xbt_cond_t cond)
{
  simcall_cond_broadcast((smx_cond_t) cond);
}

void xbt_cond_destroy(xbt_cond_t cond)
{
  SIMIX_cond_destroy((smx_cond_t) cond);
}

/***** barrier related functions *****/
typedef struct s_xbt_bar_ {
  xbt_mutex_t mutex;
  xbt_cond_t cond;
  unsigned int arrived_processes;
  unsigned int expected_processes;
} s_xbt_bar_;

xbt_bar_t xbt_barrier_init(unsigned int count)
{
  xbt_bar_t bar = xbt_new0(s_xbt_bar_, 1);
  bar->expected_processes = count;
  bar->arrived_processes = 0;
  bar->mutex = xbt_mutex_init();
  bar->cond = xbt_cond_init();
  return bar;
}

int xbt_barrier_wait(xbt_bar_t bar)
{
   int ret=0;
   xbt_mutex_acquire(bar->mutex);
   if (++bar->arrived_processes == bar->expected_processes) {
     xbt_cond_broadcast(bar->cond);
     xbt_mutex_release(bar->mutex);
     ret=XBT_BARRIER_SERIAL_PROCESS;
     bar->arrived_processes = 0;
   } else {
     xbt_cond_wait(bar->cond, bar->mutex);
     xbt_mutex_release(bar->mutex);
   }
   return ret;
}

void xbt_barrier_destroy(xbt_bar_t bar)
{
   xbt_mutex_destroy(bar->mutex);
   xbt_cond_destroy(bar->cond);
   xbt_free(bar);
}
