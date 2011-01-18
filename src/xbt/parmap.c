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

  DEBUG1("Create new parmap (%u workers)", num_workers);

  /* Initialize the thread pool data structure */
  xbt_parmap_t parmap = xbt_new0(s_xbt_parmap_t, 1);
  parmap->num_workers = num_workers;
  parmap->status = PARMAP_WORK;

  parmap->workers_ready = xbt_new0(s_xbt_barrier_t, 1);
  xbt_barrier_init(parmap->workers_ready, num_workers + 1);
  parmap->workers_done = xbt_new0(s_xbt_barrier_t, 1);
  xbt_barrier_init(parmap->workers_done, num_workers + 1);
#ifndef HAVE_FUTEX_H
  parmap->workers_ready->mutex = xbt_os_mutex_init();
  parmap->workers_ready->cond = xbt_os_cond_init();
#endif
  /* Create the pool of worker threads */
  for(i=0; i < num_workers; i++){
    worker = xbt_os_thread_create(NULL, _xbt_parmap_worker_main, parmap, NULL);
    xbt_os_thread_detach(worker);
  }
  
  return parmap;
}

void xbt_parmap_destroy(xbt_parmap_t parmap)
{ 
  DEBUG1("Destroy parmap %p", parmap);

  parmap->status = PARMAP_DESTROY;

  xbt_barrier_wait(parmap->workers_ready);
  DEBUG0("Kill job sent");
  xbt_barrier_wait(parmap->workers_done);
#ifndef HAVE_FUTEX_H
  xbt_os_mutex_destroy(parmap->workers_ready->mutex);
  xbt_os_cond_destroy(parmap->workers_ready->cond);
#endif
  xbt_free(parmap->workers_ready);
  xbt_free(parmap->workers_done);
  xbt_free(parmap);
}

 void xbt_parmap_apply(xbt_parmap_t parmap, void_f_pvoid_t fun, xbt_dynar_t data)
{
  /* Assign resources to worker threads*/
  parmap->fun = fun;
  parmap->data = data;

  /* Notify workers that there is a job */
  xbt_barrier_wait(parmap->workers_ready);
  DEBUG0("Job dispatched, lets wait...");
  xbt_barrier_wait(parmap->workers_done);

  DEBUG0("Job done");
  parmap->fun = NULL;
  parmap->data = NULL;
}

static void *_xbt_parmap_worker_main(void *arg)
{
  unsigned int data_start, data_end, data_size, worker_id;
  xbt_parmap_t parmap = (xbt_parmap_t)arg;

  /* Fetch a worker id */
  worker_id = __sync_fetch_and_add(&parmap->workers_max_id, 1);

  DEBUG1("New worker thread created (%u)", worker_id);
  
  /* Worker's main loop */
  while(1){
    xbt_barrier_wait(parmap->workers_ready);

    if(parmap->status == PARMAP_WORK){
      DEBUG1("Worker %u got a job", worker_id);

      /* Compute how much data does every worker gets */
      data_size = (xbt_dynar_length(parmap->data) / parmap->num_workers)
                  + ((xbt_dynar_length(parmap->data) % parmap->num_workers) ? 1 : 0);

      /* Each worker data segment starts in a position associated with its id*/
      data_start = data_size * worker_id;

      /* The end of the worker data segment must be bounded by the end of the data vector */
      data_end = MIN(data_start + data_size, xbt_dynar_length(parmap->data));

      DEBUG4("Worker %u: data_start=%u data_end=%u (data_size=%u)",
          worker_id, data_start, data_end, data_size);

      /* While the worker don't pass the end of it data segment apply the function */
      while(data_start < data_end){
        parmap->fun(*(void **)xbt_dynar_get_ptr(parmap->data, data_start));
        data_start++;
      }

      xbt_barrier_wait(parmap->workers_done);

    /* We are destroying the parmap */
    }else{
      xbt_barrier_wait(parmap->workers_done);
      DEBUG1("Shutting down worker %u", worker_id);
      return NULL;
    }
  }
}

#ifdef HAVE_FUTEX_H
	static void futex_wait(int *uaddr, int val)
	{
	  DEBUG1("Waiting on futex %d", *uaddr);
	  syscall(SYS_futex, uaddr, FUTEX_WAIT_PRIVATE, val, NULL, NULL, 0);
	}

	static void futex_wake(int *uaddr, int val)
	{
	  DEBUG1("Waking futex %d", *uaddr);
	  syscall(SYS_futex, uaddr, FUTEX_WAKE_PRIVATE, val, NULL, NULL, 0);
	}
#endif

/* Futex based implementation of the barrier */
void xbt_barrier_init(xbt_barrier_t barrier, unsigned int threads_to_wait)
{
  barrier->threads_to_wait = threads_to_wait;
  barrier->thread_count = 0;
}

#ifdef HAVE_FUTEX_H
	void xbt_barrier_wait(xbt_barrier_t barrier)
	{
	  int myflag = 0;
	  unsigned int mycount = 0;

	  myflag = barrier->futex;
	  mycount = __sync_add_and_fetch(&barrier->thread_count, 1);
	  if(mycount < barrier->threads_to_wait){
		futex_wait(&barrier->futex, myflag);
	  }else{
		barrier->futex = __sync_add_and_fetch(&barrier->futex, 1);
		barrier->thread_count = 0;
		futex_wake(&barrier->futex, barrier->threads_to_wait);
	  }
	}
#else
	void xbt_barrier_wait(xbt_barrier_t barrier)
	{
	  xbt_os_mutex_acquire(barrier->mutex);

	  barrier->thread_count++;
	  if(barrier->thread_count < barrier->threads_to_wait){
		  xbt_os_cond_wait(barrier->cond,barrier->mutex);
	  }else{
		barrier->thread_count = 0;
		xbt_os_cond_broadcast(barrier->cond);
	  }
	  xbt_os_mutex_release(barrier->mutex);
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
  //INFO1("I'm job %lu", (unsigned long)arg);
}

XBT_TEST_UNIT("basic", test_parmap_basic, "Basic usage")
{
  xbt_test_add0("Create the parmap");

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
}

#endif /* SIMGRID_TEST */
