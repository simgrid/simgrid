/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_LINK_ENERGY_H_
#define SIMGRID_PLUGINS_LINK_ENERGY_H_

#include <xbt/base.h>
#include <simgrid/forward.h>

SG_BEGIN_DECL()

XBT_PUBLIC(double) sg_link_get_consumped_power(sg_link_t link);
XBT_PUBLIC(double) sg_link_get_usage(sg_link_t link);
XBT_PUBLIC(void) sg_on_simulation_end();
XBT_PUBLIC(void) sg_link_energy_plugin_init();
XBT_PUBLIC(double) sg_link_get_consumed_energy(sg_link_t link);

XBT_PUBLIC(double) ns3__link_get_consumped_power(sg_link_t link);
XBT_PUBLIC(double) ns3_link_get_usage(sg_link_t link);
XBT_PUBLIC(void) ns3_on_simulation_end();
XBT_PUBLIC(double) ns3_link_get_consumed_energy(sg_link_t link);
XBT_PUBLIC(void) ns3_link_energy_plugin_init();

SG_END_DECL()

#endif

