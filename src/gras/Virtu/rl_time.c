/* $Id$ */

/* time - time related syscal wrappers                                      */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"

#include "xbt/sysdep.h"
#include "gras/virtu.h"


double gras_os_time() {
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (double)(tv.tv_sec + tv.tv_usec / 1000000);
#else
  /* Poor resolution */
  return (double)(time(NULL)); 
#endif /* HAVE_GETTIMEOFDAY? */ 
	
}
 
void gras_os_sleep(unsigned long sec,unsigned long usec) {
  sleep(sec);
  if (usec/1000000) sleep(usec/1000000);

#ifdef HAVE_USLEEP
  (void)usleep(usec % 1000000);
#endif /* ! HAVE_USLEEP */
}
