/*  xbt/xbt_os_timer.h -- system dependency on time functions               */

/* Copyright (c) 2007, 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_OS_TIMER_H
#define _XBT_OS_TIMER_H

#include <xbt/misc.h>           /* XBT_PUBLIC */
#include <stddef.h>             /* size_t */

SG_BEGIN_DECL()

/** @brief get time in seconds 
 *
 * gives  the  number  of  seconds since the Epoch (00:00:00 UTC, January 1, 1970).
 */
XBT_PUBLIC(double) xbt_os_time(void);
XBT_PUBLIC(void) xbt_os_sleep(double sec);

typedef struct s_xbt_os_timer *xbt_os_timer_t;

XBT_PUBLIC(xbt_os_timer_t) xbt_os_timer_new(void);
XBT_PUBLIC(void) xbt_os_timer_free(xbt_os_timer_t timer);
XBT_PUBLIC(double) xbt_os_timer_elapsed(xbt_os_timer_t timer);
XBT_PUBLIC(size_t) xbt_os_timer_size(void);

XBT_PUBLIC(void) xbt_os_walltimer_start(xbt_os_timer_t timer);
XBT_PUBLIC(void) xbt_os_walltimer_resume(xbt_os_timer_t timer);
XBT_PUBLIC(void) xbt_os_walltimer_stop(xbt_os_timer_t timer);

XBT_PUBLIC(void) xbt_os_cputimer_start(xbt_os_timer_t timer);
XBT_PUBLIC(void) xbt_os_cputimer_resume(xbt_os_timer_t timer);
XBT_PUBLIC(void) xbt_os_cputimer_stop(xbt_os_timer_t timer);

XBT_PUBLIC(void) xbt_os_threadtimer_start(xbt_os_timer_t timer);
XBT_PUBLIC(void) xbt_os_threadtimer_resume(xbt_os_timer_t timer);
XBT_PUBLIC(void) xbt_os_threadtimer_stop(xbt_os_timer_t timer);

SG_END_DECL()

#endif
