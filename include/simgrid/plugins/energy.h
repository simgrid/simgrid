/* Copyright (c) 2016-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_ENERGY_H_
#define SIMGRID_PLUGINS_ENERGY_H_

#include <xbt/base.h>
#include <simgrid/forward.h>

SG_BEGIN_DECL()

XBT_PUBLIC void sg_host_energy_plugin_init();
XBT_PUBLIC void sg_host_energy_update_all();
XBT_PUBLIC double sg_host_get_consumed_energy(sg_host_t host);
XBT_PUBLIC double sg_host_get_wattidle_at(sg_host_t host, int pstate);
XBT_PUBLIC double sg_host_get_wattmin_at(sg_host_t host, int pstate);
XBT_PUBLIC double sg_host_get_wattmax_at(sg_host_t host, int pstate);
XBT_PUBLIC double sg_host_get_power_range_slope_at(sg_host_t host, int pstate);
XBT_PUBLIC double sg_host_get_current_consumption(sg_host_t host);

XBT_PUBLIC void sg_link_energy_plugin_init();
XBT_PUBLIC double sg_link_get_consumed_energy(sg_link_t link);

XBT_PUBLIC int sg_link_energy_is_inited();

#define MSG_host_energy_plugin_init() sg_host_energy_plugin_init()
#define MSG_host_get_consumed_energy(host) sg_host_get_consumed_energy(host)
#define MSG_host_get_wattidle_at(host,pstate) sg_host_get_wattidle_at((host), (pstate))
#define MSG_host_get_wattmin_at(host,pstate) sg_host_get_wattmin_at((host), (pstate))
#define MSG_host_get_wattmax_at(host,pstate) sg_host_get_wattmax_at((host), (pstate))
#define MSG_host_get_power_range_slope_at(host,pstate) sg_host_get_power_range_slope_at((host), (pstate))
#define MSG_host_get_current_consumption(host) sg_host_get_current_consumption(host)

SG_END_DECL()

#endif
