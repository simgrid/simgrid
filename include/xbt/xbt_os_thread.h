/* xbt/xbt_os_thread.h -- Thread portability layer                          */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_OS_THREAD_H
#define XBT_OS_THREAD_H

#include <xbt/base.h>
#include <xbt/function_types.h>

#include <pthread.h>

SG_BEGIN_DECL()

/** @addtogroup XBT_thread
 *  @brief Thread portability layer
 *
 *  This section describes the thread portability layer. It defines types and functions very close to the pthread API,
 *  but it's portable to windows too.
 *
 *  @{
 */

/** @brief Thread mutex data type (opaque structure) */
typedef struct xbt_os_mutex_ *xbt_os_mutex_t;
XBT_PUBLIC xbt_os_mutex_t xbt_os_mutex_init(void);
XBT_PUBLIC void xbt_os_mutex_acquire(xbt_os_mutex_t mutex);
XBT_PUBLIC void xbt_os_mutex_release(xbt_os_mutex_t mutex);
XBT_PUBLIC void xbt_os_mutex_destroy(xbt_os_mutex_t mutex);

/** @} */

SG_END_DECL()
#endif /* XBT_OS_THREAD_H */
