/* virtu - virtualization layer for the logging to know about the actors    */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_VIRTU_H
#define XBT_VIRTU_H

#include <xbt/base.h>
#include <xbt/dynar.h>
#include <xbt/function_types.h>
#include <xbt/misc.h>

SG_BEGIN_DECL

XBT_PUBLIC const char* xbt_procname(void);

XBT_PUBLIC int xbt_getpid(void);

/* Get the name of the UNIX process englobing the world */
XBT_PUBLIC_DATA char* xbt_binary_name;

/** Contains all the parameters we got from the command line (including argv[0]) */
XBT_PUBLIC_DATA xbt_dynar_t xbt_cmdline;

SG_END_DECL
#endif /* XBT_VIRTU_H */
