/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef SIMGRID_PLUGINS_PHOTOVOLTAIC_H_
#define SIMGRID_PLUGINS_PHOTOVOLTAIC_H_

#include <simgrid/config.h>
#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL

XBT_PUBLIC void sg_photovoltaic_plugin_init();

XBT_PUBLIC void sg_photovoltaic_set_solar_irradiance(const_sg_host_t host, double s);

XBT_PUBLIC double sg_photovoltaic_get_power(const_sg_host_t host);

SG_END_DECL

#endif
