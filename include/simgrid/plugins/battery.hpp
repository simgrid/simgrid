/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef SIMGRID_PLUGINS_BATTERY_H_
#define SIMGRID_PLUGINS_BATTERY_H_

#include <simgrid/config.h>
#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL

XBT_PUBLIC void sg_battery_plugin_init();

XBT_PUBLIC void sg_battery_set_state(const_sg_host_t host, bool state);
XBT_PUBLIC void sg_battery_set_power(const_sg_host_t host, double power);

XBT_PUBLIC bool sg_battery_is_active(const_sg_host_t host);
XBT_PUBLIC double sg_battery_get_power(const_sg_host_t host);
XBT_PUBLIC double sg_battery_get_state_of_charge(const_sg_host_t host);
XBT_PUBLIC double sg_battery_get_state_of_charge_min(const_sg_host_t host);
XBT_PUBLIC double sg_battery_get_state_of_charge_max(const_sg_host_t host);
XBT_PUBLIC double sg_battery_get_state_of_health(const_sg_host_t host);
XBT_PUBLIC double sg_battery_get_capacity(const_sg_host_t host);
XBT_PUBLIC double sg_battery_get_cumulative_cost(const_sg_host_t host);
XBT_PUBLIC double sg_battery_get_next_event_date(const_sg_host_t host);

SG_END_DECL

#endif