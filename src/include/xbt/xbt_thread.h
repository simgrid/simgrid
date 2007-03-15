/* $Id$ */

/* xbt/xbt_thread.h -- Thread portability layer                             */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _XBT_THREAD_H
#define _XBT_THREAD_H

#include "xbt/misc.h" /* SG_BEGIN_DECL */
#include "xbt/function_types.h"

SG_BEGIN_DECL()

/** @addtogroup XBT_thread
 *  @brief Thread portability layer
 * 
 *  This section describes the thread portability layer. It defines types and 
 *  functions very close to the pthread API, but it's portable to windows too.
 * 
 *  @{
 */

  /** \brief Thread data type (opaque structure) */
  typedef struct xbt_thread_       *xbt_thread_t;

  XBT_PUBLIC(xbt_thread_t) xbt_thread_create(pvoid_f_pvoid_t start_routine,void* param);
  XBT_PUBLIC(void) xbt_thread_exit(int *retcode);
  XBT_PUBLIC(xbt_thread_t) xbt_thread_self(void);
  XBT_PUBLIC(void) xbt_thread_join(xbt_thread_t thread,void ** thread_return);
  XBT_PUBLIC(void) xbt_thread_yield(void);


  /** \brief Thread mutex data type (opaque structure) */
  typedef struct xbt_mutex_ *xbt_mutex_t;

  XBT_PUBLIC(xbt_mutex_t) xbt_mutex_init(void);
  XBT_PUBLIC(void)        xbt_mutex_lock(xbt_mutex_t mutex);
  XBT_PUBLIC(void)        xbt_mutex_unlock(xbt_mutex_t mutex);
  XBT_PUBLIC(void)        xbt_mutex_destroy(xbt_mutex_t mutex);


  /** \brief Thread condition data type (opaque structure) */
  typedef struct xbt_thcond_  *xbt_thcond_t;

  XBT_PUBLIC(xbt_thcond_t) xbt_thcond_init(void);
  XBT_PUBLIC(void)         xbt_thcond_wait(xbt_thcond_t cond,
					   xbt_mutex_t mutex);
  XBT_PUBLIC(void)         xbt_thcond_signal(xbt_thcond_t cond);
  XBT_PUBLIC(void)         xbt_thcond_broadcast(xbt_thcond_t cond);
  XBT_PUBLIC(void)         xbt_thcond_destroy(xbt_thcond_t cond);

/** @} */

SG_END_DECL()

#endif /* _XBT_DICT_H */
