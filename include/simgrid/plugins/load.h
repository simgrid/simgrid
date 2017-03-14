/* Copyright (c) 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_LOAD_H_
#define SIMGRID_PLUGINS_LOAD_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL()

XBT_PUBLIC(void) sg_host_load_plugin_init();
XBT_PUBLIC(double) sg_host_get_current_load(sg_host_t host);
XBT_PUBLIC(double) sg_host_get_computed_flops(sg_host_t host);
XBT_PUBLIC(void) sg_host_load_reset(sg_host_t host);

#define MSG_host_load_plugin_init() sg_host_load_plugin_init()
#define MSG_host_get_current_load(host) sg_host_get_current_load(host)
#define MSG_host_get_computed_flops(host) sg_host_get_computed_flops(host)

SG_END_DECL()

#endif
