/* Copyright (c) 2012-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_INSTR_INTERFACE_H
#define SIMGRID_INSTR_INTERFACE_H

#include "xbt.h"

SG_BEGIN_DECL()

XBT_PUBLIC(int) TRACE_start ();
XBT_PUBLIC(int) TRACE_end ();
XBT_PUBLIC(void) TRACE_global_init();
XBT_PUBLIC(void) TRACE_help(int detailed);
XBT_PUBLIC(void) TRACE_surf_resource_utilization_alloc();
XBT_PUBLIC(void) TRACE_surf_resource_utilization_release();

SG_END_DECL()

#endif
