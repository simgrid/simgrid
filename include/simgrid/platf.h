/* platf.h - Public interface to the SimGrid platforms                      */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_H
#define SG_PLATF_H

#include <xbt.h>                /* our toolbox */

XBT_PUBLIC(void) sg_platf_new_AS_open(const char *id, const char *mode);
XBT_PUBLIC(void) sg_platf_new_AS_close(void);

#endif                          /* SG_PLATF_H */
