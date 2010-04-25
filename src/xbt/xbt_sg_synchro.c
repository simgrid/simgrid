/* xbt_synchro -- Synchronization virtualized depending on whether we are   */
/*                in simulation or real life (act on simulated processes)   */

/* This is the simulation implementation, using simix.                      */

/* Copyright 2006,2007 Malek Cherier, Martin Quinson
 * All right reserved.                                                      */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"

#include "xbt/synchro.h"        /* This module */

#include "simix/simix.h"        /* used implementation */
#include "simix/datatypes.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_sync, xbt,
                                "Synchronization mechanism");

/* the implementation would be cleaner (and faster) with ELF symbol aliasing */

typedef struct s_xbt_thread_ {
  char *name;
  smx_process_t s_process;
  void_f_pvoid_t code;
  void *userparam;
  void *father_data;
  /* stuff to allow other people to wait on me with xbt_thread_join */
  int joinable:1,done:1;
  xbt_cond_t cond;
  xbt_mutex_t mutex;
} s_xbt_thread_t;

static int xbt_thread_create_wrapper(int argc, char *argv[])
{
  xbt_thread_t t =
    (xbt_thread_t) SIMIX_process_get_data(SIMIX_process_self());
  SIMIX_process_set_data(SIMIX_process_self(), t->father_data);
  (*t->code) (t->userparam);
  if (t->joinable) {
    t->done=1;
    xbt_mutex_acquire(t->mutex);
    xbt_cond_broadcast(t->cond);
    xbt_mutex_release(t->mutex);
  } else {
    xbt_mutex_destroy(t->mutex);
    xbt_cond_destroy(t->cond);
    free(t->name);
    free(t);
  }
  return 0;
}

xbt_thread_t xbt_thread_create(const char *name, void_f_pvoid_t code,
                               void *param,int joinable)
{
  xbt_thread_t res = xbt_new0(s_xbt_thread_t, 1);
  res->name = xbt_strdup(name);
  res->userparam = param;
  res->code = code;
  res->father_data = SIMIX_process_get_data(SIMIX_process_self());
  //   char*name = bprintf("%s#%p",SIMIX_process_get_name(SIMIX_process_self()), param);
  res->s_process = SIMIX_process_create(name,
                                        xbt_thread_create_wrapper, res,
                                        SIMIX_host_get_name(SIMIX_host_self
                                                            ()), 0, NULL,
                                        /*props */ NULL);
  res->joinable = joinable;
  res->done = 0;
  res->cond = xbt_cond_init();
  res->mutex = xbt_mutex_init();
  //   free(name);
  return res;
}

const char *xbt_thread_name(xbt_thread_t t)
{
  return t->name;
}

const char *xbt_thread_self_name(void)
{
  xbt_thread_t me = xbt_thread_self();
  return me ? me->name : "maestro";
}


void xbt_thread_join(xbt_thread_t thread)
{
  xbt_mutex_acquire(thread->mutex);
  xbt_assert1(thread->joinable,"Cannot join on %p: wasn't created joinable",thread);
  if (!thread->done) {
    xbt_cond_wait(thread->cond,thread->mutex);
    xbt_mutex_release(thread->mutex);
  }

  xbt_mutex_destroy(thread->mutex);
  xbt_cond_destroy(thread->cond);
  free(thread->name);
  free(thread);

}

void xbt_thread_cancel(xbt_thread_t thread)
{
  SIMIX_process_kill(thread->s_process);
  free(thread->name);
  free(thread);
}

void xbt_thread_exit()
{
  SIMIX_process_kill(SIMIX_process_self());
}

xbt_thread_t xbt_thread_self(void)
{
  smx_process_t p = SIMIX_process_self();
  return p ? SIMIX_process_get_data(p) : NULL;
}

void xbt_thread_yield(void) {
  SIMIX_process_yield();
}

/****** mutex related functions ******/
struct s_xbt_mutex_ {

  /* KEEP IT IN SYNC WITH src/simix/private.h::struct s_smx_mutex */
  xbt_swag_t sleeping;          /* list of sleeping process */
  int refcount;
  /* KEEP IT IN SYNC WITH src/simix/private.h::struct s_smx_mutex */

};

xbt_mutex_t xbt_mutex_init(void)
{
  return (xbt_mutex_t) SIMIX_mutex_init();
}

void xbt_mutex_acquire(xbt_mutex_t mutex)
{
  SIMIX_mutex_lock((smx_mutex_t) mutex);
}

void xbt_mutex_release(xbt_mutex_t mutex)
{
  SIMIX_mutex_unlock((smx_mutex_t) mutex);
}

void xbt_mutex_destroy(xbt_mutex_t mutex)
{
  SIMIX_mutex_destroy((smx_mutex_t) mutex);
}

/***** condition related functions *****/
struct s_xbt_cond_ {

  /* KEEP IT IN SYNC WITH src/simix/private.h::struct s_smx_cond */
  xbt_swag_t sleeping;          /* list of sleeping process */
  smx_mutex_t mutex;
  xbt_fifo_t actions;           /* list of actions */
  /* KEEP IT IN SYNC WITH src/simix/private.h::struct s_smx_cond */

};

xbt_cond_t xbt_cond_init(void)
{
  return (xbt_cond_t) SIMIX_cond_init();
}

void xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex)
{
  SIMIX_cond_wait((smx_cond_t) cond, (smx_mutex_t) mutex);
}

void xbt_cond_timedwait(xbt_cond_t cond, xbt_mutex_t mutex, double delay)
{
  SIMIX_cond_wait_timeout((smx_cond_t) cond, (smx_mutex_t) mutex, delay);
}

void xbt_cond_signal(xbt_cond_t cond)
{
  SIMIX_cond_signal((smx_cond_t) cond);
}

void xbt_cond_broadcast(xbt_cond_t cond)
{
  SIMIX_cond_broadcast((smx_cond_t) cond);
}

void xbt_cond_destroy(xbt_cond_t cond)
{
  SIMIX_cond_destroy((smx_cond_t) cond);
}
