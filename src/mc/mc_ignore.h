/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_IGNORE_H
#define SIMGRID_MC_IGNORE_H

#include "src/internal_config.h"
#include "xbt/dynar.h"

#if HAVE_UCONTEXT_H
#include <ucontext.h>           /* context relative declarations */
#endif


SG_BEGIN_DECL();

XBT_PUBLIC(void) MC_ignore_heap(void *address, size_t size);
XBT_PUBLIC(void) MC_remove_ignore_heap(void *address, size_t size);
XBT_PUBLIC(void) MC_ignore_local_variable(const char *var_name, const char *frame);
XBT_PUBLIC(void) MC_ignore_global_variable(const char *var_name);

#if HAVE_UCONTEXT_H
XBT_PUBLIC(void) MC_register_stack_area(void *stack, smx_actor_t process, ucontext_t* context, size_t size);
#endif


SG_END_DECL();

#endif
