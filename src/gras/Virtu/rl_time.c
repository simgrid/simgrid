/* $Id$ */

/* time - time related syscal wrappers                                      */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <math.h> /* floor */

#include "portable.h"

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "gras/virtu.h"
#include "xbt/xbt_portability.h" /* private */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_virtu);

double gras_os_time() {
  return xbt_os_time();
}
 
void gras_os_sleep(double sec) {
#ifdef HAVE_USLEEP
  DEBUG1("Do sleep %f sec", sec);
  sleep(sec);
  (void)usleep( (sec - floor(sec)) * 1000000);
        
#else /* don't have usleep. Use select to sleep less than one second */
  struct timeval timeout;

  DEBUG1("Do sleep %f sec", sec);
  
  timeout.tv_sec =  (unsigned long)(sec);
  timeout.tv_usec = (sec - floor(sec)) * 1000000;
              
  select(0, NULL, NULL, NULL, &timeout);
#endif
}

