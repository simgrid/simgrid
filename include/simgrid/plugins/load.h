/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_LOAD_H_
#define SIMGRID_PLUGINS_LOAD_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL

XBT_PUBLIC void sg_host_load_plugin_init();
XBT_PUBLIC double sg_host_get_current_load(sg_host_t host);
XBT_PUBLIC double sg_host_get_avg_load(sg_host_t host);
XBT_PUBLIC double sg_host_get_idle_time(sg_host_t host);
XBT_PUBLIC double sg_host_get_total_idle_time(sg_host_t host);
XBT_PUBLIC double sg_host_get_computed_flops(sg_host_t host);
XBT_PUBLIC void sg_host_load_reset(sg_host_t host);

#define MSG_host_load_plugin_init() sg_host_load_plugin_init()
/** @brief Returns the current load of that host, as a ratio = achieved_flops / (core_current_speed * core_amount)
 *
 *  See simgrid::plugin::HostLoad::get_current_load() for the full documentation.
 */
#define MSG_host_get_current_load(host) sg_host_get_current_load(host)
#define MSG_host_get_computed_flops(host) sg_host_get_computed_flops(host)
#define MSG_host_get_avg_load(host) sg_host_get_avg_load(host)

SG_END_DECL

#endif
