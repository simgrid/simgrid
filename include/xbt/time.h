/* xbt/time.h -- Time tools  								                                */
/* Usable in simulator, (or in real life when mixing with GRAS)             */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"           /* SG_BEGIN_DECL */

SG_BEGIN_DECL()

/**
 *
 * Time management functions, returns the system time or sleeps a process. They work both on the simulated and real systems(GRAS). 
 */
XBT_PUBLIC(double) xbt_time(void);
XBT_PUBLIC(void) xbt_sleep(double sec);

SG_END_DECL()
