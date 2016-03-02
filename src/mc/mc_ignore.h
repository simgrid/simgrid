/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_IGNORE_H
#define SIMGRID_MC_IGNORE_H

#include <xbt/base.h>           /* SG_BEGIN_DECL */
#include <xbt/dynar.h>

SG_BEGIN_DECL();

XBT_PRIVATE xbt_dynar_t MC_checkpoint_ignore_new(void);

SG_END_DECL();

#endif
