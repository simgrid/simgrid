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
#include "simgrid/simix.h"
#include "simgrid/modelchecker.h" /* our public interface (and definition of HAVE_MC) */
#include "xbt/automaton.h"
#include "xbt/dynar.h"

#define STD_HEAP_SIZE   20480000        /* Maximum size of the system's heap */

SG_BEGIN_DECL()

extern char*_surf_mc_property_file; /* fixme: better location? */

extern xbt_dynar_t mmalloc_ignore;
extern xbt_dynar_t stacks_areas;

/********************************* Global *************************************/
void _mc_cfg_cb_reduce(const char *name, int pos);
void _mc_cfg_cb_checkpoint(const char *name, int pos);
void _mc_cfg_cb_property(const char *name, int pos);

XBT_PUBLIC(void) MC_do_the_modelcheck_for_real(void);

XBT_PUBLIC(void) MC_init_safety(void);
XBT_PUBLIC(void) MC_exit(void);
XBT_PUBLIC(void) MC_exit_liveness(void);
XBT_PUBLIC(void) MC_modelcheck(void);
XBT_PUBLIC(void) MC_modelcheck_liveness(void);
XBT_PUBLIC(void) MC_process_clock_add(smx_process_t, double);
XBT_PUBLIC(double) MC_process_clock_get(smx_process_t);
void MC_automaton_load(const char *file);

void MC_ignore_init(void);
XBT_PUBLIC(void) MC_ignore(void *address, size_t size);
void MC_new_stack_area(void *stack, char *name);

/********************************* Memory *************************************/
XBT_PUBLIC(void) MC_memory_init(void);  /* Initialize the memory subsystem */
XBT_PUBLIC(void) MC_memory_exit(void);

/********************************* Snapshot comparison test *************************************/
void MC_test_snapshot_comparison(void);

/* Trigger for state equality detection (check potential cycle in application) */
void MC_compare(void);

SG_END_DECL()

#endif                          /* _MC_MC_H */
