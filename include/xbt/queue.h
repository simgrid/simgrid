/* $Id$ */

/* A (synchronized) message queue.                                          */
/* Popping an empty queue is blocking, as well as pushing a full one        */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_QUEUE_H
#define _XBT_QUEUE_H

#include "xbt/misc.h" /* SG_BEGIN_DECL */
/* #include "xbt/function_types.h" */

SG_BEGIN_DECL()

/** @addtogroup XBT_queue
  * @brief Synchronized message exchanging queue.
  *
  * These is the classical producer/consumer synchronization scheme, which all concurrent programmer recode one day or another.
  *  
  * For performance concerns, the content of queue must be homogeneous, 
  * just like dynars (see the \ref XBT_dynar section). Indeed, queues use a 
  * dynar to store the data, and add the synchronization on top of it. 
  * 
  * @{
  */

  /** \brief Queue data type (opaque type) */
  typedef struct s_xbt_queue_ *xbt_queue_t;


  XBT_PUBLIC(xbt_queue_t)   xbt_queue_new(int capacity, unsigned long elm_size);
  XBT_PUBLIC(void)          xbt_queue_free(xbt_queue_t *queue);

  XBT_PUBLIC(unsigned long) xbt_queue_length(const xbt_queue_t queue);

  XBT_PUBLIC(void) xbt_queue_push     (xbt_queue_t queue, const void *src);
  XBT_PUBLIC(void) xbt_queue_pop      (xbt_queue_t queue, void *const dst);
  XBT_PUBLIC(void) xbt_queue_unshift (xbt_queue_t queue, const void *src);
  XBT_PUBLIC(void) xbt_queue_shift   (xbt_queue_t queue, void *const dst);

  XBT_PUBLIC(void) xbt_queue_push_timed    (xbt_queue_t queue, const void *src, double delay);
  XBT_PUBLIC(void) xbt_queue_unshift_timed (xbt_queue_t queue, const void *src, double delay);
  XBT_PUBLIC(void) xbt_queue_shift_timed   (xbt_queue_t queue, void *const dst, double delay);
  XBT_PUBLIC(void) xbt_queue_pop_timed     (xbt_queue_t queue, void *const dst, double delay);

/** @} */

SG_END_DECL()

#endif /* _XBT_QUEUE_H */
