/* virtu - virtualization layer for XBT to choose between GRAS and MSG implementation */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef __XBT_VIRTU_H__
#define __XBT_VIRTU_H__

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/function_types.h"

SG_BEGIN_DECL()

/* Get the PID of the current (simulated) process */
XBT_PUBLIC_DATA(int_f_void_t) xbt_getpid;

/* Current time */
XBT_PUBLIC(double) xbt_time(void);

    /* Get the name of the UNIX process englobing the world */
    XBT_PUBLIC(const char*) xbt_os_procname(void);

    /**
     *
     * Time management functions, returns the system time or sleeps a process. They work both on the simulated and real systems(GRAS).
     */
    XBT_PUBLIC(double) xbt_time(void);
    XBT_PUBLIC(void) xbt_sleep(double sec);


SG_END_DECL()
#endif                          /* __XBT_VIRTU_H__ */
