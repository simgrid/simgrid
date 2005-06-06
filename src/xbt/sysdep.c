/* $Id$ */

/* sysdep.c -- all system dependency                                        */
/*  no system header should be loaded out of this file so that we have only */
/*  one file to check when porting to another OS                            */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/xbt_portability.h" /* private */
#include "xbt/log.h"
#include "xbt/error.h"
#include "portable.h"


/* TSC (tick-level) timers are said to be unreliable on SMP hosts and thus 
   disabled in SDL source code */ 


/* \defgroup XBT_sysdep All system dependency
 * \brief This section describes many macros/functions that can serve as
 *  an OS abstraction.
 */

double xbt_os_time(void) {
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (double)(tv.tv_sec + tv.tv_usec / 1000000.0);
#else
  /* Poor resolution */
  return (double)(time(NULL));
#endif /* HAVE_GETTIMEOFDAY? */ 	
}

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sysdep, xbt, "System dependency");


struct s_xbt_os_timer {
#ifdef HAVE_GETTIMEOFDAY
   struct timeval start,stop;
#else
   unsigned long int start,stop;
#endif
};

xbt_os_timer_t xbt_os_timer_new(void) {
   return xbt_new0(struct s_xbt_os_timer,1);
}
void xbt_os_timer_free(xbt_os_timer_t timer) {
   free (timer);
}
void xbt_os_timer_start(xbt_os_timer_t timer) {
#ifdef HAVE_GETTIMEOFDAY
  gettimeofday(&(timer->start), NULL);
#else 
  timer->start = (unsigned long int)(time(NULL));
#endif
}
void xbt_os_timer_stop(xbt_os_timer_t timer) {
#ifdef HAVE_GETTIMEOFDAY
  gettimeofday(&(timer->stop), NULL);
#else 
  timer->stop = (unsigned long int)(time(NULL));
#endif
}
double xbt_os_timer_elapsed(xbt_os_timer_t timer) {
#ifdef HAVE_GETTIMEOFDAY
   return  ((double)timer->stop.tv_sec)  - ((double)timer->start.tv_sec) +
         ((((double)timer->stop.tv_usec) - ((double)timer->start.tv_usec)) / 1000000.0);
#else 
   return  (double)timer->stop  - (double)timer->start;
#endif
}

