/* $Id$ */

/* time - time related syscal wrappers                                      */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "math.h" /* floor */

#include "portable.h"

#include "xbt/sysdep.h"
#include "gras/virtu.h"

XBT_LOG_EXTERNAL_CATEGORY(virtu);
XBT_LOG_DEFAULT_CATEGORY(virtu);

double gras_os_time() {
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (double)(tv.tv_sec + tv.tv_usec / 1000000.0);
#else
  /* Poor resolution */
  return (double)(time(NULL)); 
#endif /* HAVE_GETTIMEOFDAY? */ 
	
}
 
void gras_os_sleep(double sec) {
  DEBUG1("Do sleep %d sec", (int)sec);
  sleep(sec);

#ifdef HAVE_USLEEP
  DEBUG1("Do sleep %d usec", (int) ((sec - floor(sec)) * 1000000 ));
  (void)usleep( (sec - floor(sec)) * 1000000);
#else
  if ( ((int) sec) == 0) {
     WARN0("This platform does not implement usleep. Cannot sleep less than one second");
  }
#endif /* ! HAVE_USLEEP */
}
