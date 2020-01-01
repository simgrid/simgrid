/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_IGNORE_HPP
#define SIMGRID_MC_IGNORE_HPP

#include "simgrid/forward.h"
#include "src/internal_config.h"

#if HAVE_UCONTEXT_H
#include <ucontext.h> /* context relative declarations */

XBT_PUBLIC void MC_register_stack_area(void* stack, ucontext_t* context, size_t size);

#endif

#endif
