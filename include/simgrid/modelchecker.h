/* simgrid/modelchecker.h - Formal Verification made possible in SimGrid    */

/* Copyright (c) 2008-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid_config.h> /* HAVE_MC ? */
#include <xbt.h>

#ifndef SIMGRID_MODELCHECKER_H
#define SIMGRID_MODELCHECKER_H

#ifdef HAVE_MC

extern int _surf_do_model_check;
#define MC_IS_ENABLED _surf_do_model_check

XBT_PUBLIC(void) MC_assert(int);
XBT_PUBLIC(void) MC_assert_stateful(int);
XBT_PUBLIC(int) MC_random(int min, int max);
XBT_PUBLIC(void) MC_diff(void);

#else

#define MC_IS_ENABLED 0
#define MC_assert(a) xbt_assert(a)
#define MC_assert_stateful(a) xbt_assert(a)

#endif



#endif /* SIMGRID_MODELCHECKER_H */
