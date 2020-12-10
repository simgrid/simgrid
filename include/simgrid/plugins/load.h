/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_LOAD_H_
#define SIMGRID_PLUGINS_LOAD_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL

XBT_PUBLIC void sg_host_load_plugin_init();
XBT_PUBLIC double sg_host_get_current_load(const_sg_host_t host);
XBT_PUBLIC double sg_host_get_avg_load(const_sg_host_t host);
XBT_PUBLIC double sg_host_get_idle_time(const_sg_host_t host);
XBT_PUBLIC double sg_host_get_total_idle_time(const_sg_host_t host);
XBT_PUBLIC double sg_host_get_computed_flops(const_sg_host_t host);
XBT_PUBLIC void sg_host_load_reset(const_sg_host_t host);

XBT_PUBLIC void sg_link_load_plugin_init();
XBT_PUBLIC void sg_link_load_track(const_sg_link_t link);
XBT_PUBLIC void sg_link_load_untrack(const_sg_link_t link);
XBT_PUBLIC void sg_link_load_reset(const_sg_link_t link);
XBT_PUBLIC double sg_link_get_cum_load(const_sg_link_t link);
XBT_PUBLIC double sg_link_get_avg_load(const_sg_link_t link);
XBT_PUBLIC double sg_link_get_min_instantaneous_load(const_sg_link_t link);
XBT_PUBLIC double sg_link_get_max_instantaneous_load(const_sg_link_t link);

#if SIMGRID_HAVE_MSG

#define MSG_host_load_plugin_init() sg_host_load_plugin_init()
/** @brief Returns the current load of that host, as a ratio = achieved_flops / (core_current_speed * core_amount)
 *
 *  See simgrid::plugin::HostLoad::get_current_load() for the full documentation.
 */
#define MSG_host_get_current_load(host) sg_host_get_current_load(host)
#define MSG_host_get_computed_flops(host) sg_host_get_computed_flops(host)
#define MSG_host_get_avg_load(host) sg_host_get_avg_load(host)

#endif // SIMGRID_HAVE_MSG

SG_END_DECL

#endif
