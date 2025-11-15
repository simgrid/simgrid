/* Copyright (c) 2023-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_CARBON_FOOTPRINT_H_
#define SIMGRID_PLUGINS_CARBON_FOOTPRINT_H_

#include <simgrid/config.h>
#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL

XBT_PUBLIC void sg_host_carbon_footprint_plugin_init();
XBT_PUBLIC double sg_host_get_carbon_footprint(const_sg_host_t host);
XBT_PUBLIC double sg_host_get_carbon_intensity(const_sg_host_t host);
XBT_PUBLIC void sg_host_set_carbon_intensity(const_sg_host_t host, double carbon_intensity);
XBT_PUBLIC void sg_host_carbon_footprint_load_trace_file(const char* trace_file);

SG_END_DECL

#endif 