/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_IGNORE_H
#define SIMGRID_MC_IGNORE_H

#include "src/internal_config.h"

#if HAVE_UCONTEXT_H
#include <ucontext.h>           /* context relative declarations */

SG_BEGIN_DECL();
XBT_PUBLIC(void) MC_register_stack_area(void *stack, smx_actor_t process, ucontext_t* context, size_t size);
SG_END_DECL();

#endif

#endif
