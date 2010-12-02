/* Copyright (c) 2004, 2005, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "threadpool_private.h"

static void *_xbt_tpool_worker_main(void *tpool);

xbt_tpool_t xbt_tpool_new(unsigned int num_workers, unsigned int max_jobs)
{
  unsigned int i;
  xbt_os_thread_t worker = NULL;

  /* Initialize thread pool data structure */
  xbt_tpool_t tpool = xbt_new0(s_xbt_tpool_t, 1);
  tpool->mutex = xbt_os_mutex_init();
  tpool->job_posted = xbt_os_cond_init();
  tpool->job_taken = xbt_os_cond_init();
  tpool->all_jobs_done = xbt_os_cond_init();
  tpool->jobs_queue = xbt_dynar_new(sizeof(s_xbt_tpool_job_t), NULL);
  tpool->num_workers = num_workers;
  tpool->max_jobs = max_jobs;
  
  /* Create the pool of worker threads */
  for(i=0; i < num_workers; i++){
    worker = xbt_os_thread_create(NULL, _xbt_tpool_worker_main, tpool, NULL);
    xbt_os_thread_detach(worker);
  }
  
  return tpool;
}

void xbt_tpool_destroy(xbt_tpool_t tpool)
{ 
  /* Lock the pool, then signal every worker an wait for each to finish */
  xbt_os_mutex_acquire(tpool->mutex);
  tpool->flags = TPOOL_DESTROY; 

  while(tpool->num_workers){
    xbt_os_cond_signal(tpool->job_posted);
    xbt_os_cond_wait(tpool->job_posted, tpool->mutex);
  }

  /* Destroy pool's data structures */
  xbt_os_cond_destroy(tpool->job_posted);
  xbt_os_cond_destroy(tpool->job_taken);
  xbt_os_cond_destroy(tpool->all_jobs_done);
  xbt_os_mutex_release(tpool->mutex);
  xbt_os_mutex_destroy(tpool->mutex);  
  xbt_free(tpool);
}

void xbt_tpool_queue_job(xbt_tpool_t tpool, void_f_pvoid_t fun, void* fun_arg)
{
  s_xbt_tpool_job_t job;
  job.fun = fun;
  job.fun_arg = fun_arg;

  /* Wait until we can lock on the pool with some space on it for the job */
  xbt_os_mutex_acquire(tpool->mutex);
  while(xbt_dynar_length(tpool->jobs_queue) == tpool->max_jobs)
    xbt_os_cond_wait(tpool->job_taken, tpool->mutex); 

  /* Push the job in the queue, signal the workers and unlock the pool */
  xbt_dynar_push_as(tpool->jobs_queue, s_xbt_tpool_job_t, job);
  xbt_os_cond_signal(tpool->job_posted);
  xbt_os_mutex_release(tpool->mutex);    
  return;
}

void xbt_tpool_wait_all(xbt_tpool_t tpool)
{
  xbt_os_mutex_acquire(tpool->mutex);
  while(xbt_dynar_length(tpool->jobs_queue))
    xbt_os_cond_wait(tpool->job_taken, tpool->mutex);
  xbt_os_mutex_release(tpool->mutex);
  
  return;
}

static void *_xbt_tpool_worker_main(void *arg)
{
  s_xbt_tpool_job_t job;
  xbt_tpool_t tpool = (xbt_tpool_t)arg;
  
  /* Worker's main loop */
  while(1){
    xbt_os_mutex_acquire(tpool->mutex);

    /* If there are no jobs in the queue wait for one */
    if(!xbt_dynar_length(tpool->jobs_queue))
      xbt_os_cond_wait(tpool->job_posted, tpool->mutex);

    /* If we are shutting down, signal the destroyer so it can kill the other */
    /* workers, unlock the pool and return  */
    if(tpool->flags == TPOOL_DESTROY){
      tpool->num_workers--;
      xbt_os_cond_signal(tpool->job_taken);
      xbt_os_mutex_release(tpool->mutex);
      return NULL;
    }

    /* Get a job, signal the pool to inform jobs submitters and unlock it */
    job = xbt_dynar_pop_as(tpool->jobs_queue, s_xbt_tpool_job_t);
    xbt_os_cond_signal(tpool->job_taken);
    xbt_os_mutex_release(tpool->mutex);
  
    /* Run the job and loop again ... */
    job.fun(job.fun_arg);
  }
}