/* Copyright (c) 2004-2005, 2007, 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <atomic>

#include "src/internal_config.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _WIN32
#include <sys/syscall.h>
#endif

#if HAVE_FUTEX_H
#include <linux/futex.h>
#include <limits.h>
#endif

#include "xbt/parmap.h"
#include "xbt/log.h"
#include "xbt/function_types.h"
#include "xbt/dynar.h"
#include "xbt/xbt_os_thread.h"
#include "xbt/sysdep.h"
#include "src/simix/smx_private.h"
#include "src/simix/smx_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_parmap, xbt, "parmap: parallel map");

typedef enum {
  XBT_PARMAP_WORK,
  XBT_PARMAP_DESTROY
} e_xbt_parmap_flag_t;

static void xbt_parmap_set_mode(xbt_parmap_t parmap, e_xbt_parmap_mode_t mode);
static void *xbt_parmap_worker_main(void *parmap);
static void xbt_parmap_work(xbt_parmap_t parmap);

static void xbt_parmap_posix_master_wait(xbt_parmap_t parmap);
static void xbt_parmap_posix_worker_signal(xbt_parmap_t parmap);
static void xbt_parmap_posix_master_signal(xbt_parmap_t parmap);
static void xbt_parmap_posix_worker_wait(xbt_parmap_t parmap, unsigned round);

#if HAVE_FUTEX_H
static void xbt_parmap_futex_master_wait(xbt_parmap_t parmap);
static void xbt_parmap_futex_worker_signal(xbt_parmap_t parmap);
static void xbt_parmap_futex_master_signal(xbt_parmap_t parmap);
static void xbt_parmap_futex_worker_wait(xbt_parmap_t parmap, unsigned round);
static void futex_wait(unsigned *uaddr, unsigned val);
static void futex_wake(unsigned *uaddr, unsigned val);
#endif

static void xbt_parmap_busy_master_wait(xbt_parmap_t parmap);
static void xbt_parmap_busy_worker_signal(xbt_parmap_t parmap);
static void xbt_parmap_busy_master_signal(xbt_parmap_t parmap);
static void xbt_parmap_busy_worker_wait(xbt_parmap_t parmap, unsigned round);

/**
 * \brief Parallel map structure
 */
typedef struct s_xbt_parmap {
  e_xbt_parmap_flag_t status;      /**< is the parmap active or being destroyed? */
  unsigned work;                   /**< index of the current round */
  unsigned thread_counter;         /**< number of workers that have done the work */

  unsigned int num_workers;        /**< total number of worker threads including the controller */
  xbt_os_thread_t *workers;        /**< worker thread handlers */
  void_f_pvoid_t fun;              /**< function to run in parallel on each element of data */
  xbt_dynar_t data;                /**< parameters to pass to fun in parallel */
  std::atomic<unsigned int> index; /**< index of the next element of data to pick */

  /* posix only */
  xbt_os_cond_t ready_cond;
  xbt_os_mutex_t ready_mutex;
  xbt_os_cond_t done_cond;
  xbt_os_mutex_t done_mutex;

  /* fields that depend on the synchronization mode */
  e_xbt_parmap_mode_t mode;        /**< synchronization mode */
  void (*master_wait_f)(xbt_parmap_t);    /**< wait for the workers to have done the work */
  void (*worker_signal_f)(xbt_parmap_t);  /**< signal the master that a worker has done the work */
  void (*master_signal_f)(xbt_parmap_t);  /**< wakes the workers threads to process tasks */
  void (*worker_wait_f)(xbt_parmap_t, unsigned); /**< waits for more work */
} s_xbt_parmap_t;

/**
 * \brief Thread data transmission structure
 */
typedef struct s_xbt_parmap_thread_data{
  xbt_parmap_t parmap;
  int worker_id;
} s_xbt_parmap_thread_data_t;

typedef s_xbt_parmap_thread_data_t *xbt_parmap_thread_data_t;

/**
 * \brief Creates a parallel map object
 * \param num_workers number of worker threads to create
 * \param mode how to synchronize the worker threads
 * \return the parmap created
 */
xbt_parmap_t xbt_parmap_new(unsigned int num_workers, e_xbt_parmap_mode_t mode)
{
  XBT_DEBUG("Create new parmap (%u workers)", num_workers);

  /* Initialize the thread pool data structure */
  xbt_parmap_t parmap = new s_xbt_parmap_t();
  parmap->workers = xbt_new(xbt_os_thread_t, num_workers);

  parmap->num_workers = num_workers;
  parmap->status = XBT_PARMAP_WORK;
  xbt_parmap_set_mode(parmap, mode);

  /* Create the pool of worker threads */
  xbt_parmap_thread_data_t data;
  parmap->workers[0] = NULL;
#if HAVE_PTHREAD_SETAFFINITY
  int core_bind = 0;
#endif  
  for (unsigned int i = 1; i < num_workers; i++) {
    data = xbt_new0(s_xbt_parmap_thread_data_t, 1);
    data->parmap = parmap;
    data->worker_id = i;
    parmap->workers[i] = xbt_os_thread_create(NULL, xbt_parmap_worker_main, data, NULL);
#if HAVE_PTHREAD_SETAFFINITY
    xbt_os_thread_bind(parmap->workers[i], core_bind); 
    if (core_bind != xbt_os_get_numcores())
      core_bind++;
    else
      core_bind = 0; 
#endif
  }
  return parmap;
}

/**
 * \brief Destroys a parmap
 * \param parmap the parmap to destroy
 */
void xbt_parmap_destroy(xbt_parmap_t parmap)
{
  if (!parmap) {
    return;
  }

  parmap->status = XBT_PARMAP_DESTROY;
  parmap->master_signal_f(parmap);

  unsigned int i;
  for (i = 1; i < parmap->num_workers; i++)
    xbt_os_thread_join(parmap->workers[i], NULL);

  xbt_os_cond_destroy(parmap->ready_cond);
  xbt_os_mutex_destroy(parmap->ready_mutex);
  xbt_os_cond_destroy(parmap->done_cond);
  xbt_os_mutex_destroy(parmap->done_mutex);

  xbt_free(parmap->workers);
  delete parmap;
}

/**
 * \brief Sets the synchronization mode of a parmap.
 * \param parmap a parallel map object
 * \param mode the synchronization mode
 */
static void xbt_parmap_set_mode(xbt_parmap_t parmap, e_xbt_parmap_mode_t mode)
{
  if (mode == XBT_PARMAP_DEFAULT) {
#if HAVE_FUTEX_H
    mode = XBT_PARMAP_FUTEX;
#else
    mode = XBT_PARMAP_POSIX;
#endif
  }
  parmap->mode = mode;

  switch (mode) {
    case XBT_PARMAP_POSIX:
      parmap->master_wait_f = xbt_parmap_posix_master_wait;
      parmap->worker_signal_f = xbt_parmap_posix_worker_signal;
      parmap->master_signal_f = xbt_parmap_posix_master_signal;
      parmap->worker_wait_f = xbt_parmap_posix_worker_wait;

      parmap->ready_cond = xbt_os_cond_init();
      parmap->ready_mutex = xbt_os_mutex_init();
      parmap->done_cond = xbt_os_cond_init();
      parmap->done_mutex = xbt_os_mutex_init();
      break;
    case XBT_PARMAP_FUTEX:
#if HAVE_FUTEX_H
      parmap->master_wait_f = xbt_parmap_futex_master_wait;
      parmap->worker_signal_f = xbt_parmap_futex_worker_signal;
      parmap->master_signal_f = xbt_parmap_futex_master_signal;
      parmap->worker_wait_f = xbt_parmap_futex_worker_wait;

      xbt_os_cond_destroy(parmap->ready_cond);
      xbt_os_mutex_destroy(parmap->ready_mutex);
      xbt_os_cond_destroy(parmap->done_cond);
      xbt_os_mutex_destroy(parmap->done_mutex);
      break;
#else
      xbt_die("Futex is not available on this OS.");
#endif
    case XBT_PARMAP_BUSY_WAIT:
      parmap->master_wait_f = xbt_parmap_busy_master_wait;
      parmap->worker_signal_f = xbt_parmap_busy_worker_signal;
      parmap->master_signal_f = xbt_parmap_busy_master_signal;
      parmap->worker_wait_f = xbt_parmap_busy_worker_wait;

      xbt_os_cond_destroy(parmap->ready_cond);
      xbt_os_mutex_destroy(parmap->ready_mutex);
      xbt_os_cond_destroy(parmap->done_cond);
      xbt_os_mutex_destroy(parmap->done_mutex);
      break;
    case XBT_PARMAP_DEFAULT:
      THROW_IMPOSSIBLE;
      break;
  }
}

/**
 * \brief Applies a list of tasks in parallel.
 * \param parmap a parallel map object
 * \param fun the function to call in parallel
 * \param data each element of this dynar will be passed as an argument to fun
 */
void xbt_parmap_apply(xbt_parmap_t parmap, void_f_pvoid_t fun, xbt_dynar_t data)
{
  /* Assign resources to worker threads (we are maestro here)*/
  parmap->fun = fun;
  parmap->data = data;
  parmap->index = 0;
  parmap->master_signal_f(parmap); // maestro runs futex_wait to wake all the minions (the working threads)
  xbt_parmap_work(parmap);         // maestro works with its minions
  parmap->master_wait_f(parmap);   // When there is no more work to do, then maestro waits for the last minion to stop
  XBT_DEBUG("Job done");           //   ... and proceeds
}

/**
 * \brief Returns a next task to process.
 *
 * Worker threads call this function to get more work.
 *
 * \return the next task to process, or NULL if there is no more work
 */
void* xbt_parmap_next(xbt_parmap_t parmap)
{
  unsigned int index = parmap->index++;
  if (index < xbt_dynar_length(parmap->data)) {
    return xbt_dynar_get_as(parmap->data, index, void*);
  }
  return NULL;
}

static void xbt_parmap_work(xbt_parmap_t parmap)
{
  unsigned index;
  while ((index = parmap->index++) < xbt_dynar_length(parmap->data))
    parmap->fun(xbt_dynar_get_as(parmap->data, index, void*));
}

/**
 * \brief Main function of a worker thread.
 * \param arg the parmap
 */
static void *xbt_parmap_worker_main(void *arg)
{
  xbt_parmap_thread_data_t data = (xbt_parmap_thread_data_t) arg;
  xbt_parmap_t parmap = data->parmap;
  unsigned round = 0;
  smx_context_t context = SIMIX_context_new(NULL, 0, NULL, NULL, NULL);
  SIMIX_context_set_current(context);

  XBT_DEBUG("New worker thread created");

  /* Worker's main loop */
  while (1) {
    parmap->worker_wait_f(parmap, ++round);
    if (parmap->status == XBT_PARMAP_WORK) {
      XBT_DEBUG("Worker %d got a job", data->worker_id);

      xbt_parmap_work(parmap);
      parmap->worker_signal_f(parmap);

      XBT_DEBUG("Worker %d has finished", data->worker_id);
    /* We are destroying the parmap */
    } else {
      SIMIX_context_free(context);
      xbt_free(data);
      return NULL;
    }
  }
}

#if HAVE_FUTEX_H
static void futex_wait(unsigned *uaddr, unsigned val)
{
  XBT_VERB("Waiting on futex %p", uaddr);
  syscall(SYS_futex, uaddr, FUTEX_WAIT_PRIVATE, val, NULL, NULL, 0);
}

static void futex_wake(unsigned *uaddr, unsigned val)
{
  XBT_VERB("Waking futex %p", uaddr);
  syscall(SYS_futex, uaddr, FUTEX_WAKE_PRIVATE, val, NULL, NULL, 0);
}
#endif

/**
 * \brief Starts the parmap: waits for all workers to be ready and returns.
 *
 * This function is called by the controller thread.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_posix_master_wait(xbt_parmap_t parmap)
{
  xbt_os_mutex_acquire(parmap->done_mutex);
  if (parmap->thread_counter < parmap->num_workers) {
    /* wait for all workers to be ready */
    xbt_os_cond_wait(parmap->done_cond, parmap->done_mutex);
  }
  xbt_os_mutex_release(parmap->done_mutex);
}

/**
 * \brief Ends the parmap: wakes the controller thread when all workers terminate.
 *
 * This function is called by all worker threads when they end (not including the controller).
 *
 * \param parmap a parmap
 */
static void xbt_parmap_posix_worker_signal(xbt_parmap_t parmap)
{
  xbt_os_mutex_acquire(parmap->done_mutex);
  if (++parmap->thread_counter == parmap->num_workers) {
    /* all workers have finished, wake the controller */
    xbt_os_cond_signal(parmap->done_cond);
  }
  xbt_os_mutex_release(parmap->done_mutex);
}

/**
 * \brief Wakes all workers and waits for them to finish the tasks.
 *
 * This function is called by the controller thread.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_posix_master_signal(xbt_parmap_t parmap)
{
  xbt_os_mutex_acquire(parmap->ready_mutex);
  parmap->thread_counter = 1;
  parmap->work++;
  /* wake all workers */
  xbt_os_cond_broadcast(parmap->ready_cond);
  xbt_os_mutex_release(parmap->ready_mutex);
}

/**
 * \brief Waits for some work to process.
 *
 * This function is called by each worker thread (not including the controller) when it has no more work to do.
 *
 * \param parmap a parmap
 * \param round  the expected round number
 */
static void xbt_parmap_posix_worker_wait(xbt_parmap_t parmap, unsigned round)
{
  xbt_os_mutex_acquire(parmap->ready_mutex);
  /* wait for more work */
  if (parmap->work != round) {
    xbt_os_cond_wait(parmap->ready_cond, parmap->ready_mutex);
  }
  xbt_os_mutex_release(parmap->ready_mutex);
}

#if HAVE_FUTEX_H
/**
 * \brief Starts the parmap: waits for all workers to be ready and returns.
 *
 * This function is called by the controller thread.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_futex_master_wait(xbt_parmap_t parmap)
{
  unsigned count = parmap->thread_counter;
  while (count < parmap->num_workers) {
    /* wait for all workers to be ready */
    futex_wait(&parmap->thread_counter, count);
    count = parmap->thread_counter;
  }
}

/**
 * \brief Ends the parmap: wakes the controller thread when all workers terminate.
 *
 * This function is called by all worker threads when they end (not including the controller).
 *
 * \param parmap a parmap
 */
static void xbt_parmap_futex_worker_signal(xbt_parmap_t parmap)
{
  unsigned count = __sync_add_and_fetch(&parmap->thread_counter, 1);
  if (count == parmap->num_workers) {
    /* all workers have finished, wake the controller */
    futex_wake(&parmap->thread_counter, INT_MAX);
  }
}

/**
 * \brief Wakes all workers and waits for them to finish the tasks.
 *
 * This function is called by the controller thread.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_futex_master_signal(xbt_parmap_t parmap)
{
  parmap->thread_counter = 1;
  __sync_add_and_fetch(&parmap->work, 1);
  /* wake all workers */
  futex_wake(&parmap->work, INT_MAX);
}

/**
 * \brief Waits for some work to process.
 *
 * This function is called by each worker thread (not including the controller) when it has no more work to do.
 *
 * \param parmap a parmap
 * \param round  the expected round number
 */
static void xbt_parmap_futex_worker_wait(xbt_parmap_t parmap, unsigned round)
{
  unsigned work = parmap->work;
  /* wait for more work */
  while (work != round) {
    futex_wait(&parmap->work, work);
    work = parmap->work;
  }
}
#endif

/**
 * \brief Starts the parmap: waits for all workers to be ready and returns.
 *
 * This function is called by the controller thread.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_busy_master_wait(xbt_parmap_t parmap)
{
  while (parmap->thread_counter < parmap->num_workers) {
    xbt_os_thread_yield();
  }
}

/**
 * \brief Ends the parmap: wakes the controller thread when all workers terminate.
 *
 * This function is called by all worker threads when they end.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_busy_worker_signal(xbt_parmap_t parmap)
{
  __sync_add_and_fetch(&parmap->thread_counter, 1);
}

/**
 * \brief Wakes all workers and waits for them to finish the tasks.
 *
 * This function is called by the controller thread.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_busy_master_signal(xbt_parmap_t parmap)
{
  parmap->thread_counter = 1;
  __sync_add_and_fetch(&parmap->work, 1);
}

/**
 * \brief Waits for some work to process.
 *
 * This function is called by each worker thread (not including the controller) when it has no more work to do.
 *
 * \param parmap a parmap
 * \param round  the expected round number
 */
static void xbt_parmap_busy_worker_wait(xbt_parmap_t parmap, unsigned round)
{
  /* wait for more work */
  while (parmap->work != round) {
    xbt_os_thread_yield();
  }
}
