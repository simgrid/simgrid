/* Copyright (c) 2017-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_DVFS_H_
#define SIMGRID_PLUGINS_DVFS_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL

XBT_PUBLIC void sg_host_dvfs_plugin_init();

#define MSG_host_dvfs_plugin_init() sg_host_dvfs_plugin_init()

SG_END_DECL

#endif
