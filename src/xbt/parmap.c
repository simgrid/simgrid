/* Copyright (c) 2004, 2005, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "gras_config.h"
#include <unistd.h>
#ifndef _XBT_WIN32
#include <sys/syscall.h>
#endif

#ifdef HAVE_FUTEX_H
	#include <linux/futex.h>
#else
	#include "xbt/xbt_os_thread.h"
#endif
#include <errno.h>
#include "parmap_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_parmap, xbt, "parmap: parallel map");
XBT_LOG_NEW_SUBCATEGORY(xbt_parmap_unit, xbt_parmap, "parmap unit testing");

static void *_xbt_parmap_worker_main(void *parmap);
#ifdef HAVE_FUTEX_H
	static void futex_wait(int *uaddr, int val);
	static void futex_wake(int *uaddr, int val);
#endif
xbt_parmap_t xbt_parmap_new(unsigned int num_workers)
{
  unsigned int i;
  xbt_os_thread_t worker = NULL;

  XBT_DEBUG("Create new parmap (%u workers)", num_workers);

  /* Initialize the thread pool data structure */
  xbt_parmap_t parmap = xbt_new0(s_xbt_parmap_t, 1);
#ifdef HAVE_FUTEX_H
  parmap->sync_event = xbt_new0(s_xbt_event_t, 1);
#endif
  parmap->num_workers = num_workers;
  parmap->status = PARMAP_WORK;
#ifdef HAVE_FUTEX_H
  parmap->sync_event->threads_to_wait = num_workers;
#endif
  /* Create the pool of worker threads */
  for(i=0; i < num_workers; i++){
    worker = xbt_os_thread_create(NULL, _xbt_parmap_worker_main, parmap, NULL);
    xbt_os_thread_detach(worker);
  }
#ifdef HAVE_FUTEX_H
  xbt_event_init(parmap->sync_event);
#endif
  return parmap;
}

void xbt_parmap_destroy(xbt_parmap_t parmap)
{ 
  parmap->status = PARMAP_DESTROY;
#ifdef HAVE_FUTEX_H
  xbt_event_signal(parmap->sync_event);
  xbt_free(parmap->sync_event);
#endif
  xbt_free(parmap);
}

 void xbt_parmap_apply(xbt_parmap_t parmap, void_f_pvoid_t fun, xbt_dynar_t data)
{
  /* Assign resources to worker threads*/
  parmap->fun = fun;
  parmap->data = data;
  parmap->index = 0;
#ifdef HAVE_FUTEX_H
  xbt_event_signal(parmap->sync_event);
#endif
  XBT_DEBUG("Job done");
}

void* xbt_parmap_next(xbt_parmap_t parmap) {

  unsigned int index = __sync_fetch_and_add(&parmap->index, 1);
  if (index < xbt_dynar_length(parmap->data)) {
    return xbt_dynar_get_as(parmap->data, index, void*);
  }
  return NULL;
}

unsigned long xbt_parmap_get_worker_id(xbt_parmap_t parmap) {
  return (unsigned long) xbt_os_thread_get_extra_data();
}

static void *_xbt_parmap_worker_main(void *arg)
{
  unsigned int worker_id;
  xbt_parmap_t parmap = (xbt_parmap_t) arg;

  /* Fetch a worker id */
  worker_id = __sync_fetch_and_add(&parmap->workers_max_id, 1);
  xbt_os_thread_set_extra_data((void*) (unsigned long) worker_id);

  XBT_DEBUG("New worker thread created (%u)", worker_id);
  
  /* Worker's main loop */
  while (1) {
#ifdef HAVE_FUTEX_H
    xbt_event_wait(parmap->sync_event);
#endif
    if (parmap->status == PARMAP_WORK) {

      XBT_DEBUG("Worker %u got a job", worker_id);

      void* work = xbt_parmap_next(parmap);
      if (work != NULL) {
        parmap->fun(work);
      }

      XBT_DEBUG("Worker %u has finished", worker_id);

    /* We are destroying the parmap */
    } else {
#ifdef HAVE_FUTEX_H
      xbt_event_end(parmap->sync_event);
#endif
      XBT_DEBUG("Shutting down worker %u", worker_id);
      return NULL;
    }
  }
}

#ifdef HAVE_FUTEX_H
static void futex_wait(int *uaddr, int val)
{
  XBT_VERB("Waiting on futex %p", uaddr);
  syscall(SYS_futex, uaddr, FUTEX_WAIT_PRIVATE, val, NULL, NULL, 0);
}

static void futex_wake(int *uaddr, int val)
{
  XBT_VERB("Waking futex %p", uaddr);
  syscall(SYS_futex, uaddr, FUTEX_WAKE_PRIVATE, val, NULL, NULL, 0);
}

void xbt_event_init(xbt_event_t event)
{
  int myflag = event->done;
  if(event->thread_counter < event->threads_to_wait)
    futex_wait(&event->done, myflag);
}

void xbt_event_signal(xbt_event_t event)
{
  int myflag = event->done;
  event->thread_counter = 0;
  event->work++;
  futex_wake(&event->work, event->threads_to_wait);
  futex_wait(&event->done, myflag);
}

void xbt_event_wait(xbt_event_t event)
{
  int myflag;
  unsigned int mycount;

  myflag = event->work;
  mycount = __sync_add_and_fetch(&event->thread_counter, 1);
  if(mycount == event->threads_to_wait){
    event->done++;
    futex_wake(&event->done, 1);
  }

  futex_wait(&event->work, myflag);
}

void xbt_event_end(xbt_event_t event)
{
  unsigned int mycount;

  mycount = __sync_add_and_fetch(&event->thread_counter, 1);
  if(mycount == event->threads_to_wait){
    event->done++;
    futex_wake(&event->done, 1);
  }
}
#endif

#ifdef SIMGRID_TEST
#include "xbt.h"
#include "xbt/ex.h"

XBT_TEST_SUITE("parmap", "Parallel Map");
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_parmap_unit);



xbt_parmap_t parmap;

void fun(void *arg);

void fun(void *arg)
{
  //XBT_INFO("I'm job %lu", (unsigned long)arg);
}

XBT_TEST_UNIT("basic", test_parmap_basic, "Basic usage")
{
  xbt_test_add("Create the parmap");

  unsigned long i,j;
  xbt_dynar_t data = xbt_dynar_new(sizeof(void *), NULL);

  /* Create the parallel map */
  parmap = xbt_parmap_new(10);

  for(j=0; j < 100; j++){
    xbt_dynar_push_as(data, void *, (void *)j);
  }

  for(i=0; i < 5; i++)
    xbt_parmap_apply(parmap, fun, data);

  /* Destroy the parmap */
  xbt_parmap_destroy(parmap);
  xbt_dynar_free(&data);
}

#endif /* SIMGRID_TEST */
