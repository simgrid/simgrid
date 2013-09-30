/* module - modularize the code                                             */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_MODULE_H
#define _XBT_MODULE_H

#include <xbt/misc.h>           /* XBT_PUBLIC */

#ifdef __cplusplus
extern "C" {
#endif
XBT_PUBLIC(void) xbt_init(int *argc, char **argv);
XBT_PUBLIC(void) xbt_exit(void);
#ifdef __cplusplus
}
#endif

#endif                          /* _XBT_MODULE_H */
