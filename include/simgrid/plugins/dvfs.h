/* Copyright (c) 2017-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_DVFS_H_
#define SIMGRID_PLUGINS_DVFS_H_

#include <simgrid/config.h>
#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL

XBT_PUBLIC void sg_host_dvfs_plugin_init();

#if SIMGRID_HAVE_MSG
#define MSG_host_dvfs_plugin_init() sg_host_dvfs_plugin_init()
#endif // SIMGRID_HAVE_MSG

SG_END_DECL

#endif
