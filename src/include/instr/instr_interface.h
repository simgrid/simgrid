/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt.h"

SG_BEGIN_DECL()

XBT_PUBLIC(int) TRACE_start (void);
XBT_PUBLIC(int) TRACE_end (void);
XBT_PUBLIC(void) TRACE_global_init(int *argc, char **argv);
XBT_PUBLIC(void) TRACE_help(int detailed);
XBT_PUBLIC(void) TRACE_surf_resource_utilization_alloc(void);
XBT_PUBLIC(void) TRACE_surf_resource_utilization_release(void);
XBT_PUBLIC(void) TRACE_add_start_function(void (*func)(void));
XBT_PUBLIC(void) TRACE_add_end_function(void (*func)(void));

SG_END_DECL()
