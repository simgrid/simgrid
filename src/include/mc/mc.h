/* Copyright (c) 2008-2012. Da SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _MC_MC_H
#define _MC_MC_H

#include "xbt/misc.h"
#include "xbt/fifo.h"
#include "xbt/dict.h"
#include "xbt/function_types.h"
#include "mc/datatypes.h"
#include "simix/datatypes.h"
#include "simgrid/modelchecker.h" /* our public interface (and definition of HAVE_MC) */
#include "xbt/automaton.h"


SG_BEGIN_DECL()

/********************************* Global *************************************/
XBT_PUBLIC(void) MC_init_safety_stateless(void);
XBT_PUBLIC(void) MC_init_safety_stateful(void);
XBT_PUBLIC(void) MC_exit(void);
XBT_PUBLIC(void) MC_exit_liveness(void);
XBT_PUBLIC(void) MC_assert(int);
XBT_PUBLIC(void) MC_assert_stateful(int);
XBT_PUBLIC(void) MC_modelcheck(void);
XBT_PUBLIC(void) MC_modelcheck_stateful(void);
XBT_PUBLIC(void) MC_modelcheck_liveness(xbt_automaton_t a, char *prgm);
XBT_PUBLIC(int) MC_random(int, int);
XBT_PUBLIC(void) MC_process_clock_add(smx_process_t, double);
XBT_PUBLIC(double) MC_process_clock_get(smx_process_t);
XBT_PUBLIC(void) MC_diff(void);

/********************************* Memory *************************************/
XBT_PUBLIC(void) MC_memory_init(void);  /* Initialize the memory subsystem */
XBT_PUBLIC(void) MC_memory_exit(void);

SG_END_DECL()

#endif                          /* _MC_MC_H */
