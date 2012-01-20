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
#endif

#include "xbt/parmap.h"
#include "xbt/log.h"
#include "xbt/function_types.h"
#include "xbt/dynar.h"
#include "xbt/xbt_os_thread.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_parmap, xbt, "parmap: parallel map");
XBT_LOG_NEW_SUBCATEGORY(xbt_parmap_unit, xbt_parmap, "parmap unit testing");

typedef enum {
  XBT_PARMAP_WORK = 0,
  XBT_PARMAP_DESTROY
} e_xbt_parmap_flag_t;

static void xbt_parmap_set_mode(xbt_parmap_t parmap, e_xbt_parmap_mode_t mode);
static void *xbt_parmap_worker_main(void *parmap);

static void xbt_parmap_posix_start(xbt_parmap_t parmap);
static void xbt_parmap_posix_end(xbt_parmap_t parmap);
static void xbt_parmap_posix_signal(xbt_parmap_t parmap);
static void xbt_parmap_posix_wait(xbt_parmap_t parmap);

#ifdef HAVE_FUTEX_H
static void xbt_parmap_futex_start(xbt_parmap_t parmap);
static void xbt_parmap_futex_end(xbt_parmap_t parmap);
static void xbt_parmap_futex_signal(xbt_parmap_t parmap);
static void xbt_parmap_futex_wait(xbt_parmap_t parmap);
static void futex_wait(int *uaddr, int val);
static void futex_wake(int *uaddr, int val);
#endif

static void xbt_parmap_busy_start(xbt_parmap_t parmap);
static void xbt_parmap_busy_end(xbt_parmap_t parmap);
static void xbt_parmap_busy_signal(xbt_parmap_t parmap);
static void xbt_parmap_busy_wait(xbt_parmap_t parmap);


/**
 * \brief Parallel map structure
 */
typedef struct s_xbt_parmap {
  e_xbt_parmap_flag_t status;      /**< is the parmap active or being destroyed? */
  int work;                        /**< index of the current round (1 is the first) */
  int done;                        /**< number of rounds already done (futexes only) */
  unsigned int thread_counter;     /**< number of threads currently working */
  unsigned int num_workers;        /**< total number of worker threads including the controller */
  void_f_pvoid_t fun;              /**< function to run in parallel on each element of data */
  xbt_dynar_t data;                /**< parameters to pass to fun in parallel */
  unsigned int index;              /**< index of the next element of data to pick */

  /* fields that depend on the synchronization mode */
  e_xbt_parmap_mode_t mode;        /**< synchronization mode */
  void (*start_f)(xbt_parmap_t);   /**< initializes the worker threads */
  void (*end_f)(xbt_parmap_t);     /**< finalizes the worker threads */
  void (*signal_f)(xbt_parmap_t);  /**< wakes the workers threads to process tasks */
  void (*wait_f)(xbt_parmap_t);    /**< waits for more work */
} s_xbt_parmap_t;

/**
 * \brief Creates a parallel map object
 * \param num_workers number of worker threads to create
 * \param mode how to synchronize the worker threads
 * \return the parmap created
 */
xbt_parmap_t xbt_parmap_new(unsigned int num_workers, e_xbt_parmap_mode_t mode)
{
  unsigned int i;
  xbt_os_thread_t worker = NULL;

  XBT_DEBUG("Create new parmap (%u workers)", num_workers);

  /* Initialize the thread pool data structure */
  xbt_parmap_t parmap = xbt_new0(s_xbt_parmap_t, 1);

  parmap->num_workers = num_workers;
  parmap->status = XBT_PARMAP_WORK;
  xbt_parmap_set_mode(parmap, mode);

  /* Create the pool of worker threads */
  for (i = 0; i < num_workers - 1; i++) {
    worker = xbt_os_thread_create(NULL, xbt_parmap_worker_main, parmap, NULL);
    xbt_os_thread_detach(worker);
  }
  parmap->start_f(parmap);
  return parmap;
}

/**
 * \brief Destroys a parmap
 * \param parmap the parmap to destroy
 */
void xbt_parmap_destroy(xbt_parmap_t parmap)
{
  parmap->status = XBT_PARMAP_DESTROY;
  parmap->signal_f(parmap);
  xbt_free(parmap);
}

/**
 * \brief Sets the synchronization mode of a parmap.
 * \param parmap a parallel map object
 * \param mode the synchronization mode
 */
static void xbt_parmap_set_mode(xbt_parmap_t parmap, e_xbt_parmap_mode_t mode)
{
  if (mode == XBT_PARMAP_DEFAULT) {
#ifdef HAVE_FUTEX_H
    mode = XBT_PARMAP_FUTEX;
#else
    mode = XBT_PARMAP_POSIX;
#endif
  }
  parmap->mode = mode;

  switch (mode) {

    case XBT_PARMAP_POSIX:
      parmap->start_f = xbt_parmap_posix_start;
      parmap->end_f = xbt_parmap_posix_end;
      parmap->signal_f = xbt_parmap_posix_signal;
      parmap->wait_f = xbt_parmap_posix_wait;
      break;


    case XBT_PARMAP_FUTEX:
#ifdef HAVE_FUTEX_H
      parmap->start_f = xbt_parmap_futex_start;
      parmap->end_f = xbt_parmap_futex_end;
      parmap->signal_f = xbt_parmap_futex_signal;
      parmap->wait_f = xbt_parmap_futex_wait;
      break;
#else
      xbt_die("Futex is not available on this OS (maybe you are on a Mac).");
#endif

    case XBT_PARMAP_BUSY_WAIT:
      parmap->start_f = xbt_parmap_busy_start;
      parmap->end_f = xbt_parmap_busy_end;
      parmap->signal_f = xbt_parmap_busy_signal;
      parmap->wait_f = xbt_parmap_busy_wait;
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
  /* Assign resources to worker threads */
  parmap->fun = fun;
  parmap->data = data;
  parmap->index = 0;
  parmap->signal_f(parmap);
  XBT_DEBUG("Job done");
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
  unsigned int index = __sync_fetch_and_add(&parmap->index, 1);
  if (index < xbt_dynar_length(parmap->data)) {
    return xbt_dynar_get_as(parmap->data, index, void*);
  }
  return NULL;
}

/**
 * \brief Main function of a worker thread.
 * \param arg the parmap
 */
static void *xbt_parmap_worker_main(void *arg)
{
  xbt_parmap_t parmap = (xbt_parmap_t) arg;

  XBT_DEBUG("New worker thread created");

  /* Worker's main loop */
  while (1) {
    parmap->wait_f(parmap);
    if (parmap->status == XBT_PARMAP_WORK) {

      XBT_DEBUG("Worker got a job");

      void* work = xbt_parmap_next(parmap);
      while (work != NULL) {
        parmap->fun(work);
        work = xbt_parmap_next(parmap);
      }

      XBT_DEBUG("Worker has finished");

    /* We are destroying the parmap */
    } else {
      parmap->end_f(parmap);
      XBT_DEBUG("Shutting down worker");
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
#endif

static void xbt_parmap_posix_start(xbt_parmap_t parmap)
{
  THROW_UNIMPLEMENTED;
}

static void xbt_parmap_posix_end(xbt_parmap_t parmap)
{
  THROW_UNIMPLEMENTED;
}

static void xbt_parmap_posix_signal(xbt_parmap_t parmap)
{
  THROW_UNIMPLEMENTED;
}

static void xbt_parmap_posix_wait(xbt_parmap_t parmap)
{
  THROW_UNIMPLEMENTED;
}

#ifdef HAVE_FUTEX_H
/**
 * \brief Starts the parmap: waits for all workers to be ready and returns.
 *
 * This function is called by the controller thread.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_futex_start(xbt_parmap_t parmap)
{
  int myflag = parmap->done;
  __sync_fetch_and_add(&parmap->thread_counter, 1);
  if (parmap->thread_counter < parmap->num_workers) {
    /* wait for all workers to be ready */
    futex_wait(&parmap->done, myflag);
  }
}

/**
 * \brief Ends the parmap: wakes the controller thread when all workers terminate.
 *
 * This function is called by all worker threads when they end (not including
 * the controller).
 *
 * \param parmap a parmap
 */
static void xbt_parmap_futex_end(xbt_parmap_t parmap)
{
  unsigned int mycount;

  mycount = __sync_add_and_fetch(&parmap->thread_counter, 1);
  if (mycount == parmap->num_workers) {
    /* all workers have finished, wake the controller */
    parmap->done++;
    futex_wake(&parmap->done, 1);
  }
}

/**
 * \brief Wakes all workers and waits for them to finish the tasks.
 *
 * This function is called by the controller thread.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_futex_signal(xbt_parmap_t parmap)
{
  int myflag = parmap->done;
  parmap->thread_counter = 0;
  parmap->work++;

  /* wake all workers */
  futex_wake(&parmap->work, parmap->num_workers);

  if (parmap->status == XBT_PARMAP_WORK) {
    /* also work myself */
    void* work = xbt_parmap_next(parmap);
    while (work != NULL) {
      parmap->fun(work);
      work = xbt_parmap_next(parmap);
    }
  }

  unsigned int mycount = __sync_add_and_fetch(&parmap->thread_counter, 1);
  if (mycount < parmap->num_workers) {
    /* some workers have not finished yet */
    futex_wait(&parmap->done, myflag);
  }
}

/**
 * \brief Waits for some work to process.
 *
 * This function is called by each worker thread (not including the controller)
 * when it has no more work to do.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_futex_wait(xbt_parmap_t parmap)
{
  int myflag;
  unsigned int mycount;

  myflag = parmap->work;
  mycount = __sync_add_and_fetch(&parmap->thread_counter, 1);
  if (mycount == parmap->num_workers) {
    /* all workers have finished, wake the controller */
    parmap->done++;
    futex_wake(&parmap->done, 1);
  }

  /* wait for more work */
  futex_wait(&parmap->work, myflag);
}
#endif

/**
 * \brief Starts the parmap: waits for all workers to be ready and returns.
 *
 * This function is called by the controller thread.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_busy_start(xbt_parmap_t parmap)
{
  __sync_fetch_and_add(&parmap->thread_counter, 1);
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
static void xbt_parmap_busy_end(xbt_parmap_t parmap)
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
static void xbt_parmap_busy_signal(xbt_parmap_t parmap)
{
  parmap->thread_counter = 0;
  parmap->work++;

  if (parmap->status == XBT_PARMAP_WORK) {
    /* also work myself */
    void* work = xbt_parmap_next(parmap);
    while (work != NULL) {
      parmap->fun(work);
      work = xbt_parmap_next(parmap);
    }
  }

  /* I have finished, wait for the others */
  __sync_add_and_fetch(&parmap->thread_counter, 1);
  while (parmap->thread_counter < parmap->num_workers) {
    xbt_os_thread_yield();
  }
}

/**
 * \brief Waits for some work to process.
 *
 * This function is called by each worker thread (not including the controller)
 * when it has no more work to do.
 *
 * \param parmap a parmap
 */
static void xbt_parmap_busy_wait(xbt_parmap_t parmap)
{
  int work = parmap->work;
  __sync_add_and_fetch(&parmap->thread_counter, 1);

  /* wait for more work */
  while (parmap->work == work) {
    xbt_os_thread_yield();
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
  //XBT_INFO("I'm job %lu", (unsigned long)arg);
}

XBT_TEST_UNIT("basic", test_parmap_basic, "Basic usage")
{
  xbt_test_add("Create the parmap");

  unsigned long i, j;
  xbt_dynar_t data = xbt_dynar_new(sizeof(void *), NULL);

  /* Create the parallel map */
#ifdef HAVE_FUTEX_H
  parmap = xbt_parmap_new(10, XBT_PARMAP_FUTEX);
#else
  parmap = xbt_parmap_new(10, XBT_PARMAP_BUSY_WAIT);
#endif
  for (j = 0; j < 100; j++) {
    xbt_dynar_push_as(data, void *, (void *)j);
  }

  for (i = 0; i < 5; i++) {
    xbt_parmap_apply(parmap, fun, data);
  }

  /* Destroy the parmap */
  xbt_parmap_destroy(parmap);
  xbt_dynar_free(&data);
}

#endif /* SIMGRID_TEST */
