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
#include <limits.h>
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

#ifdef HAVE_FUTEX_H
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
  void_f_pvoid_t fun;              /**< function to run in parallel on each element of data */
  xbt_dynar_t data;                /**< parameters to pass to fun in parallel */
  unsigned int index;              /**< index of the next element of data to pick */

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
  for (i = 1; i < num_workers; i++) {
    worker = xbt_os_thread_create(NULL, xbt_parmap_worker_main, parmap, NULL);
    xbt_os_thread_detach(worker);
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
  parmap->master_wait_f(parmap);

  xbt_os_cond_destroy(parmap->ready_cond);
  xbt_os_mutex_destroy(parmap->ready_mutex);
  xbt_os_cond_destroy(parmap->done_cond);
  xbt_os_mutex_destroy(parmap->done_mutex);

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
#ifdef HAVE_FUTEX_H
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
  /* Assign resources to worker threads */
  parmap->fun = fun;
  parmap->data = data;
  parmap->index = 0;
  parmap->master_signal_f(parmap);
  xbt_parmap_work(parmap);
  parmap->master_wait_f(parmap);
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

static void xbt_parmap_work(xbt_parmap_t parmap)
{
  unsigned index;
  while ((index = __sync_fetch_and_add(&parmap->index, 1))
         < xbt_dynar_length(parmap->data))
    parmap->fun(xbt_dynar_get_as(parmap->data, index, void*));
}

/**
 * \brief Main function of a worker thread.
 * \param arg the parmap
 */
static void *xbt_parmap_worker_main(void *arg)
{
  xbt_parmap_t parmap = (xbt_parmap_t) arg;
  unsigned round = 0;

  XBT_DEBUG("New worker thread created");

  /* Worker's main loop */
  while (1) {
    parmap->worker_wait_f(parmap, ++round);
    if (parmap->status == XBT_PARMAP_WORK) {

      XBT_DEBUG("Worker got a job");

      xbt_parmap_work(parmap);
      parmap->worker_signal_f(parmap);

      XBT_DEBUG("Worker has finished");

    /* We are destroying the parmap */
    } else {
      parmap->worker_signal_f(parmap);
      return NULL;
    }
  }
}

#ifdef HAVE_FUTEX_H
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
 * This function is called by all worker threads when they end (not including
 * the controller).
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
 * This function is called by each worker thread (not including the controller)
 * when it has no more work to do.
 *
 * \param parmap a parmap
 * \param round  the expected round number
 */
static void xbt_parmap_posix_worker_wait(xbt_parmap_t parmap, unsigned round)
{
  xbt_os_mutex_acquire(parmap->ready_mutex);
  /* wait for more work */
  if (parmap->work < round) {
    xbt_os_cond_wait(parmap->ready_cond, parmap->ready_mutex);
  }
  xbt_os_mutex_release(parmap->ready_mutex);
}

#ifdef HAVE_FUTEX_H
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
 * This function is called by all worker threads when they end (not including
 * the controller).
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
 * This function is called by each worker thread (not including the controller)
 * when it has no more work to do.
 *
 * \param parmap a parmap
 * \param round  the expected round number
 */
static void xbt_parmap_futex_worker_wait(xbt_parmap_t parmap, unsigned round)
{
  unsigned work = parmap->work;
  /* wait for more work */
  if (work < round)
    futex_wait(&parmap->work, work);
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
 * This function is called by each worker thread (not including the controller)
 * when it has no more work to do.
 *
 * \param parmap a parmap
 * \param round  the expected round number
 */
static void xbt_parmap_busy_worker_wait(xbt_parmap_t parmap, unsigned round)
{
  /* wait for more work */
  while (parmap->work < round) {
    xbt_os_thread_yield();
  }
}

#ifdef SIMGRID_TEST
#include "xbt.h"
#include "xbt/ex.h"
#include "xbt/xbt_os_thread.h"
#include "xbt/xbt_os_time.h"
#include "gras_config.h"        /* HAVE_FUTEX_H */

XBT_TEST_SUITE("parmap", "Parallel Map");
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_parmap_unit);

#ifdef HAVE_FUTEX_H
#define TEST_PARMAP_SKIP_TEST(mode) 0
#else
#define TEST_PARMAP_SKIP_TEST(mode) ((mode) == XBT_PARMAP_FUTEX)
#endif

#define TEST_PARMAP_VALIDATE_MODE(mode) \
  if (TEST_PARMAP_SKIP_TEST(mode)) { xbt_test_skip(); return; } else ((void)0)

static void fun_double(void *arg)
{
  unsigned *u = arg;
  *u = 2 * *u + 1;
}

/* Check that the computations are correctly done. */
static void test_parmap_basic(e_xbt_parmap_mode_t mode)
{
  unsigned num_workers;

  for (num_workers = 1 ; num_workers <= 16 ; num_workers *= 2) {
    const unsigned len = 1033;
    const unsigned num = 5;
    unsigned *a;
    xbt_dynar_t data;
    xbt_parmap_t parmap;
    unsigned i;

    xbt_test_add("Basic parmap usage (%u workers)", num_workers);

    TEST_PARMAP_VALIDATE_MODE(mode);
    parmap = xbt_parmap_new(num_workers, mode);

    a = xbt_malloc(len * sizeof *a);
    data = xbt_dynar_new(sizeof a, NULL);
    for (i = 0; i < len; i++) {
      a[i] = i;
      xbt_dynar_push_as(data, void *, &a[i]);
    }

    for (i = 0; i < num; i++)
      xbt_parmap_apply(parmap, fun_double, data);

    for (i = 0; i < len; i++) {
      unsigned expected = (1U << num) * (i + 1) - 1;
      xbt_test_assert(a[i] == expected,
                      "a[%u]: expected %u, got %u", i, expected, a[i]);
    }

    xbt_dynar_free(&data);
    xbt_free(a);
    xbt_parmap_destroy(parmap);
  }
}

XBT_TEST_UNIT("basic_posix", test_parmap_basic_posix, "Basic usage: posix")
{
  test_parmap_basic(XBT_PARMAP_POSIX);
}

XBT_TEST_UNIT("basic_futex", test_parmap_basic_futex, "Basic usage: futex")
{
  test_parmap_basic(XBT_PARMAP_FUTEX);
}

XBT_TEST_UNIT("basic_busy_wait", test_parmap_basic_busy_wait, "Basic usage: busy_wait")
{
  test_parmap_basic(XBT_PARMAP_BUSY_WAIT);
}

static void fun_get_id(void *arg)
{
  *(uintptr_t *)arg = (uintptr_t)xbt_os_thread_self();
  xbt_os_sleep(0.5);
}

static int fun_compare(const void *pa, const void *pb)
{
  uintptr_t a = *(uintptr_t *)pa;
  uintptr_t b = *(uintptr_t *)pb;
  return a < b ? -1 : a > b ? 1 : 0;
}

/* Check that all threads are working. */
static void test_parmap_extended(e_xbt_parmap_mode_t mode)
{
  unsigned num_workers;

  for (num_workers = 1 ; num_workers <= 16 ; num_workers *= 2) {
    const unsigned len = 2 * num_workers;
    uintptr_t *a;
    xbt_parmap_t parmap;
    xbt_dynar_t data;
    unsigned i;
    unsigned count;

    xbt_test_add("Extended parmap usage (%u workers)", num_workers);

    TEST_PARMAP_VALIDATE_MODE(mode);
    parmap = xbt_parmap_new(num_workers, mode);

    a = xbt_malloc(len * sizeof *a);
    data = xbt_dynar_new(sizeof a, NULL);
    for (i = 0; i < len; i++)
      xbt_dynar_push_as(data, void *, &a[i]);

    xbt_parmap_apply(parmap, fun_get_id, data);

    qsort(a, len, sizeof a[0], fun_compare);
    count = 1;
    for (i = 1; i < len; i++)
      if (a[i] != a[i - 1])
        count++;
    xbt_test_assert(count == num_workers,
                    "only %u/%u threads did some work", count, num_workers);

    xbt_dynar_free(&data);
    xbt_free(a);
    xbt_parmap_destroy(parmap);
  }
}

XBT_TEST_UNIT("extended_posix", test_parmap_extended_posix, "Extended usage: posix")
{
  test_parmap_extended(XBT_PARMAP_POSIX);
}

XBT_TEST_UNIT("extended_futex", test_parmap_extended_futex, "Extended usage: futex")
{
  test_parmap_extended(XBT_PARMAP_FUTEX);
}

XBT_TEST_UNIT("extended_busy_wait", test_parmap_extended_busy_wait, "Extended usage: busy_wait")
{
  test_parmap_extended(XBT_PARMAP_BUSY_WAIT);
}

#endif /* SIMGRID_TEST */
