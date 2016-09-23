/* virtu - virtualization layer for XBT to choose between GRAS and MSG implementation */

/* Copyright (c) 2007, 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef __XBT_VIRTU_H__
#define __XBT_VIRTU_H__

#include "xbt/misc.h"
#include "xbt/base.h"
#include "xbt/function_types.h"
#include "xbt/dynar.h"

SG_BEGIN_DECL()

/* Get the PID of the current (simulated) process */
XBT_PUBLIC_DATA(int_f_void_t) xbt_getpid;

/* Get the name of the UNIX process englobing the world */
XBT_PUBLIC_DATA(char*) xbt_binary_name;

/** Contains all the parameters we got from the command line (including argv[0]) */
XBT_PUBLIC_DATA(xbt_dynar_t) xbt_cmdline;

SG_END_DECL()
#endif                          /* __XBT_VIRTU_H__ */
