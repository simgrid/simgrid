/* $Id$ */

/* time - time related syscal wrappers                                      */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "gras/virtu.h"
#include <sys/time.h>   /* gettimeofday() */

double gras_os_time() {
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (double)(tv.tv_sec * 1000000 + tv.tv_usec);
}
 
void gras_os_sleep(unsigned long sec,unsigned long usec) {
  sleep(sec);
  if (usec/1000000) sleep(usec/1000000);

#ifdef HAVE_USLEEP
  (void)usleep(usec % 1000000);
#endif /* ! HAVE_USLEEP */
}
