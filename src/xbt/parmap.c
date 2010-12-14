/* Copyright (c) 2004, 2005, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "parmap_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_parmap, xbt, "parmap: parallel map");
XBT_LOG_NEW_SUBCATEGORY(xbt_parmap_unit, xbt_parmap, "parmap unit testing");

static void *_xbt_parmap_worker_main(void *parmap);

xbt_parmap_t xbt_parmap_new(unsigned int num_workers)
{
  unsigned int i;
  xbt_os_thread_t worker = NULL;

  DEBUG1("Create new parmap (%u workers)", num_workers);

  /* Initialize thread pool data structure */
  xbt_parmap_t parmap = xbt_new0(s_xbt_parmap_t, 1);
  parmap->mutex = xbt_os_mutex_init();
  parmap->job_posted = xbt_os_cond_init();
  parmap->all_done = xbt_os_cond_init();
  parmap->flags = xbt_new0(e_xbt_parmap_flag_t, num_workers + 1);
  parmap->num_workers = num_workers;
  parmap->num_idle_workers = 0;
  parmap->workers_max_id = 0;

  /* Init our flag to wait (for workers' initialization) */
  parmap->flags[num_workers] = PARMAP_WAIT;

  /* Create the pool of worker threads */
  for(i=0; i < num_workers; i++){
    worker = xbt_os_thread_create(NULL, _xbt_parmap_worker_main, parmap, NULL);
    xbt_os_thread_detach(worker);
  }
  
  /* wait for the workers to initialize */
  xbt_os_mutex_acquire(parmap->mutex);
  while(parmap->flags[num_workers] == PARMAP_WAIT)
    xbt_os_cond_wait(parmap->all_done, parmap->mutex);
  xbt_os_mutex_release(parmap->mutex);

  return parmap;
}

void xbt_parmap_destroy(xbt_parmap_t parmap)
{ 
  DEBUG1("Destroy parmap %p", parmap);

  unsigned int i;

  /* Lock the parmap, then signal every worker an wait for each to finish */
  xbt_os_mutex_acquire(parmap->mutex);
  for(i=0; i < parmap->num_workers; i++){
    parmap->flags[i] = PARMAP_DESTROY;
  }

  xbt_os_cond_broadcast(parmap->job_posted);
  while(parmap->num_workers){
    DEBUG1("Still %u workers, waiting...", parmap->num_workers);
    xbt_os_cond_wait(parmap->all_done, parmap->mutex);
  }

  /* Destroy pool's data structures */
  xbt_os_cond_destroy(parmap->job_posted);
  xbt_os_cond_destroy(parmap->all_done);
  xbt_free(parmap->flags);
  xbt_os_mutex_release(parmap->mutex);
  xbt_os_mutex_destroy(parmap->mutex);
  xbt_free(parmap);
}

void xbt_parmap_apply(xbt_parmap_t parmap, void_f_pvoid_t fun, xbt_dynar_t data)
{
  unsigned int i;
  unsigned int myflag_idx = parmap->num_workers;

  /* Assign resources to worker threads */
  xbt_os_mutex_acquire(parmap->mutex);
  parmap->fun = fun;
  parmap->data = data;
  parmap->num_idle_workers = 0;

  /* Set worker flags to work */
  for(i=0; i < parmap->num_workers; i++){
    parmap->flags[i] = PARMAP_WORK;
  }

  /* Set our flag to wait (for the job to be completed)*/
  parmap->flags[myflag_idx] = PARMAP_WAIT;

  /* Notify workers that there is a job */
  xbt_os_cond_broadcast(parmap->job_posted);
  DEBUG0("Job dispatched, lets wait...");

  /* wait for the workers to finish */
  while(parmap->flags[myflag_idx] == PARMAP_WAIT)
    xbt_os_cond_wait(parmap->all_done, parmap->mutex);

  DEBUG0("Job done");
  parmap->fun = NULL;
  parmap->data = NULL;

  xbt_os_mutex_release(parmap->mutex);    
  return;
}

static void *_xbt_parmap_worker_main(void *arg)
{
  unsigned int data_start, data_end, data_size, worker_id;
  xbt_parmap_t parmap = (xbt_parmap_t)arg;

  /* Fetch a worker id */
  xbt_os_mutex_acquire(parmap->mutex);
  worker_id = parmap->workers_max_id++;
  xbt_os_mutex_release(parmap->mutex);

  DEBUG1("New worker thread created (%u)", worker_id);
  
  /* Worker's main loop */
  while(1){
    xbt_os_mutex_acquire(parmap->mutex);
    parmap->flags[worker_id] = PARMAP_WAIT;
    parmap->num_idle_workers++;

    /* If everybody is done set the parmap work flag and signal it */
    if(parmap->num_idle_workers == parmap->num_workers){
      DEBUG1("Worker %u: All done, signal the parmap", worker_id);
      parmap->flags[parmap->num_workers] = PARMAP_WORK;
      xbt_os_cond_signal(parmap->all_done);
    }

    /* If the wait flag is set then ... wait. */
    while(parmap->flags[worker_id] == PARMAP_WAIT)
      xbt_os_cond_wait(parmap->job_posted, parmap->mutex);

    DEBUG1("Worker %u got a job", worker_id);

    /* If we are shutting down, the last worker is going to signal the
     * parmap so it can finish destroying the data structure */
    if(parmap->flags[worker_id] == PARMAP_DESTROY){
      DEBUG1("Shutting down worker %u", worker_id);
      parmap->num_workers--;
      if(parmap->num_workers == 0)
        xbt_os_cond_signal(parmap->all_done);
      xbt_os_mutex_release(parmap->mutex);
      return NULL;
    }
    xbt_os_mutex_release(parmap->mutex);

    /* Compute how much data does every worker gets */
    data_size = (xbt_dynar_length(parmap->data) / parmap->num_workers)
                + ((xbt_dynar_length(parmap->data) % parmap->num_workers) ? 1 : 0);

    /* Each worker data segment starts in a position associated with its id*/
    data_start = data_size * worker_id;

    /* The end of the worker data segment must be bounded by the end of the data vector */
    data_end = MIN(data_start + data_size, xbt_dynar_length(parmap->data));

    DEBUG4("Worker %u: data_start=%u data_end=%u (data_size=%u)", worker_id, data_start, data_end, data_size);

    /* While the worker don't pass the end of it data segment apply the function */
    while(data_start < data_end){
      parmap->fun(*(void **)xbt_dynar_get_ptr(parmap->data, data_start));
      data_start++;
    }
  }
}

#ifdef SIMGRID_TEST
#include "xbt.h"
#include "xbt/ex.h"

XBT_TEST_SUITE("parmap", "Parallel Map");
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_parmap_unit);



xbt_parmap_t parmap;

void fun(void *arg);

void fun(void *arg)
{
  INFO1("I'm job %lu", (unsigned long)arg);
}

XBT_TEST_UNIT("basic", test_parmap_basic, "Basic usage")
{
  xbt_test_add0("Create the parmap");

  unsigned long j;
  xbt_dynar_t data = xbt_dynar_new(sizeof(void *), NULL);

  /* Create the parallel map */
  parmap = xbt_parmap_new(5);

  for(j=0; j < 200; j++){
    xbt_dynar_push_as(data, void *, (void *)j);
  }

  xbt_parmap_apply(parmap, fun, data);

  /* Destroy the parmap */
  xbt_parmap_destroy(parmap);
}

#endif /* SIMGRID_TEST */
