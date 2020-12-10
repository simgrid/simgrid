/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_ENERGY_H_
#define SIMGRID_PLUGINS_ENERGY_H_

#include <xbt/base.h>
#include <simgrid/forward.h>

SG_BEGIN_DECL

XBT_PUBLIC void sg_host_energy_plugin_init();
XBT_PUBLIC void sg_host_energy_update_all();
XBT_PUBLIC double sg_host_get_consumed_energy(const_sg_host_t host);
XBT_PUBLIC double sg_host_get_idle_consumption(const_sg_host_t host);
XBT_PUBLIC double sg_host_get_idle_consumption_at(const_sg_host_t host, int pstate);
XBT_PUBLIC double sg_host_get_wattmin_at(const_sg_host_t host, int pstate);
XBT_PUBLIC double sg_host_get_wattmax_at(const_sg_host_t host, int pstate);
XBT_PUBLIC double sg_host_get_power_range_slope_at(const_sg_host_t host, int pstate);
XBT_PUBLIC double sg_host_get_current_consumption(const_sg_host_t host);

XBT_PUBLIC void sg_link_energy_plugin_init();
XBT_PUBLIC double sg_link_get_consumed_energy(const_sg_link_t link);

XBT_PUBLIC int sg_link_energy_is_inited();

XBT_PUBLIC void sg_wifi_energy_plugin_init();

#if SIMGRID_HAVE_MSG

#define MSG_host_energy_plugin_init() sg_host_energy_plugin_init()
#define MSG_host_get_consumed_energy(host) sg_host_get_consumed_energy(host)
#define MSG_host_get_idle_consumption_at(host,pstate) sg_host_get_idle_consumption_at((host), (pstate))
#define MSG_host_get_wattmin_at(host,pstate) sg_host_get_wattmin_at((host), (pstate))
#define MSG_host_get_wattmax_at(host,pstate) sg_host_get_wattmax_at((host), (pstate))
#define MSG_host_get_power_range_slope_at(host,pstate) sg_host_get_power_range_slope_at((host), (pstate))
#define MSG_host_get_current_consumption(host) sg_host_get_current_consumption(host)

#endif // SIMGRID_HAVE_MSG

SG_END_DECL

#endif
