/* A (synchronized) message queue.                                          */
/* Popping an empty queue is blocking, as well as pushing a full one        */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"

#include "xbt/synchro.h"
#include "xbt/queue.h"          /* this module */
#include "gras/virtu.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_queue, xbt, "Message exchanging queue");

typedef struct s_xbt_queue_ {
  int capacity;
  xbt_dynar_t data;
  xbt_mutex_t mutex;
  xbt_cond_t not_full, not_empty;
} s_xbt_queue_t;

/** @brief Create a new message exchange queue.
 *
 * @param capacity the capacity of the queue. If non-nul, any attempt to push an item which would let the size of the queue over this number will be blocking until someone else pop some data
 * @param elm_size size of each element stored in it (see #xbt_dynar_new)
 */
xbt_queue_t xbt_queue_new(int capacity, unsigned long elm_size)
{
  xbt_queue_t res = xbt_new0(s_xbt_queue_t, 1);
  xbt_assert0(capacity >= 0, "Capacity cannot be negative");

  res->capacity = capacity;
  res->data = xbt_dynar_new(elm_size, NULL);
  res->mutex = xbt_mutex_init();
  res->not_full = xbt_cond_init();
  res->not_empty = xbt_cond_init();
  return res;
}

/** @brief Destroy a message exchange queue.
 *
 * Any remaining content is leaked.
 */
void xbt_queue_free(xbt_queue_t * queue)
{

  xbt_dynar_free(&((*queue)->data));
  xbt_mutex_destroy((*queue)->mutex);
  xbt_cond_destroy((*queue)->not_full);
  xbt_cond_destroy((*queue)->not_empty);
  free((*queue));
  *queue = NULL;
}

/** @brief Get the queue size */
unsigned long xbt_queue_length(const xbt_queue_t queue)
{
  unsigned long res;
  xbt_mutex_acquire(queue->mutex);
  res = xbt_dynar_length(queue->data);
  xbt_mutex_release(queue->mutex);
  return res;
}

/** @brief Push something to the message exchange queue.
 *
 * This is blocking if the declared capacity is non-nul, and if this amount is reached.
 *
 * @see #xbt_dynar_push
 */
void xbt_queue_push(xbt_queue_t queue, const void *src)
{
  xbt_mutex_acquire(queue->mutex);
  while (queue->capacity != 0
         && queue->capacity == xbt_dynar_length(queue->data)) {
    DEBUG2("Capacity of %p exceded (=%d). Waiting", queue, queue->capacity);
    xbt_cond_wait(queue->not_full, queue->mutex);
  }
  xbt_dynar_push(queue->data, src);
  xbt_cond_signal(queue->not_empty);
  xbt_mutex_release(queue->mutex);
}


/** @brief Pop something from the message exchange queue.
 *
 * This is blocking if the queue is empty.
 *
 * @see #xbt_dynar_pop
 *
 */
void xbt_queue_pop(xbt_queue_t queue, void *const dst)
{
  xbt_mutex_acquire(queue->mutex);
  while (xbt_dynar_length(queue->data) == 0) {
    DEBUG1("Queue %p empty. Waiting", queue);
    xbt_cond_wait(queue->not_empty, queue->mutex);
  }
  xbt_dynar_pop(queue->data, dst);
  xbt_cond_signal(queue->not_full);
  xbt_mutex_release(queue->mutex);
}

/** @brief Unshift something to the message exchange queue.
 *
 * This is blocking if the declared capacity is non-nul, and if this amount is reached.
 *
 * @see #xbt_dynar_unshift
 */
void xbt_queue_unshift(xbt_queue_t queue, const void *src)
{
  xbt_mutex_acquire(queue->mutex);
  while (queue->capacity != 0
         && queue->capacity == xbt_dynar_length(queue->data)) {
    DEBUG2("Capacity of %p exceded (=%d). Waiting", queue, queue->capacity);
    xbt_cond_wait(queue->not_full, queue->mutex);
  }
  xbt_dynar_unshift(queue->data, src);
  xbt_cond_signal(queue->not_empty);
  xbt_mutex_release(queue->mutex);
}


/** @brief Shift something from the message exchange queue.
 *
 * This is blocking if the queue is empty.
 *
 * @see #xbt_dynar_shift
 *
 */
void xbt_queue_shift(xbt_queue_t queue, void *const dst)
{
  xbt_mutex_acquire(queue->mutex);
  while (xbt_dynar_length(queue->data) == 0) {
    DEBUG1("Queue %p empty. Waiting", queue);
    xbt_cond_wait(queue->not_empty, queue->mutex);
  }
  xbt_dynar_shift(queue->data, dst);
  xbt_cond_signal(queue->not_full);
  xbt_mutex_release(queue->mutex);
}




/** @brief Push something to the message exchange queue, with a timeout.
 *
 * @see #xbt_queue_push
 */
void xbt_queue_push_timed(xbt_queue_t queue, const void *src, double delay)
{
  double begin = xbt_time();
  xbt_ex_t e;

  xbt_mutex_acquire(queue->mutex);

  if (delay == 0) {
    if (queue->capacity != 0 &&
        queue->capacity == xbt_dynar_length(queue->data)) {

      xbt_mutex_release(queue->mutex);
      THROW2(timeout_error, 0, "Capacity of %p exceded (=%d), and delay = 0",
             queue, queue->capacity);
    }
  } else {
    while (queue->capacity != 0 &&
           queue->capacity == xbt_dynar_length(queue->data) &&
           (delay < 0 || (xbt_time() - begin) <= delay)) {

      DEBUG2("Capacity of %p exceded (=%d). Waiting", queue, queue->capacity);
      TRY {
        xbt_cond_timedwait(queue->not_full, queue->mutex,
                           delay < 0 ? -1 : delay - (xbt_time() - begin));
      }
      CATCH(e) {
        xbt_mutex_release(queue->mutex);
        RETHROW;
      }
    }
  }

  xbt_dynar_push(queue->data, src);
  xbt_cond_signal(queue->not_empty);
  xbt_mutex_release(queue->mutex);
}


/** @brief Pop something from the message exchange queue, with a timeout.
 *
 * @see #xbt_queue_pop
 *
 */
void xbt_queue_pop_timed(xbt_queue_t queue, void *const dst, double delay)
{
  double begin = xbt_time();
  xbt_ex_t e;

  xbt_mutex_acquire(queue->mutex);

  if (delay == 0) {
    if (xbt_dynar_length(queue->data) == 0) {
      xbt_mutex_release(queue->mutex);
      THROW0(timeout_error, 0, "Delay = 0, and queue is empty");
    }
  } else {
    while ((xbt_dynar_length(queue->data) == 0) &&
           (delay < 0 || (xbt_time() - begin) <= delay)) {
      DEBUG1("Queue %p empty. Waiting", queue);
      TRY {
        xbt_cond_timedwait(queue->not_empty, queue->mutex,
                           delay < 0 ? -1 : delay - (xbt_time() - begin));
      }
      CATCH(e) {
        xbt_mutex_release(queue->mutex);
        RETHROW;
      }
    }
  }

  xbt_dynar_pop(queue->data, dst);
  xbt_cond_signal(queue->not_full);
  xbt_mutex_release(queue->mutex);
}

/** @brief Unshift something to the message exchange queue, with a timeout.
 *
 * @see #xbt_queue_unshift
 */
void xbt_queue_unshift_timed(xbt_queue_t queue, const void *src, double delay)
{
  double begin = xbt_time();
  xbt_ex_t e;

  xbt_mutex_acquire(queue->mutex);

  if (delay == 0) {
    if (queue->capacity != 0 &&
        queue->capacity == xbt_dynar_length(queue->data)) {

      xbt_mutex_release(queue->mutex);
      THROW2(timeout_error, 0, "Capacity of %p exceded (=%d), and delay = 0",
             queue, queue->capacity);
    }
  } else {
    while (queue->capacity != 0 &&
           queue->capacity == xbt_dynar_length(queue->data) &&
           (delay < 0 || (xbt_time() - begin) <= delay)) {

      DEBUG2("Capacity of %p exceded (=%d). Waiting", queue, queue->capacity);
      TRY {
        xbt_cond_timedwait(queue->not_full, queue->mutex,
                           delay < 0 ? -1 : delay - (xbt_time() - begin));
      }
      CATCH(e) {
        xbt_mutex_release(queue->mutex);
        RETHROW;
      }
    }
  }

  xbt_dynar_unshift(queue->data, src);
  xbt_cond_signal(queue->not_empty);
  xbt_mutex_release(queue->mutex);
}


/** @brief Shift something from the message exchange queue, with a timeout.
 *
 * @see #xbt_queue_shift
 *
 */
void xbt_queue_shift_timed(xbt_queue_t queue, void *const dst, double delay)
{
  double begin = xbt_time();
  xbt_ex_t e;

  xbt_mutex_acquire(queue->mutex);

  if (delay == 0) {
    if (xbt_dynar_length(queue->data) == 0) {
      xbt_mutex_release(queue->mutex);
      THROW0(timeout_error, 0, "Delay = 0, and queue is empty");
    }
  } else {
    while ((xbt_dynar_length(queue->data) == 0) &&
           (delay < 0 || (xbt_time() - begin) <= delay)) {
      DEBUG1("Queue %p empty. Waiting", queue);
      TRY {
        xbt_cond_timedwait(queue->not_empty, queue->mutex,
                           delay < 0 ? -1 : delay - (xbt_time() - begin));
      }
      CATCH(e) {
        xbt_mutex_release(queue->mutex);
        RETHROW;
      }
    }
  }

  if (xbt_dynar_length(queue->data) == 0) {
    xbt_mutex_release(queue->mutex);
    THROW1(timeout_error, 0, "Timeout (%f) elapsed, but queue still empty",
           delay);
  }

  xbt_dynar_shift(queue->data, dst);
  xbt_cond_signal(queue->not_full);
  xbt_mutex_release(queue->mutex);
}
