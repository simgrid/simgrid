/* Copyright (c) 2012-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_CONFIG_H
#define SIMGRID_CONFIG_H

#include "xbt/config.h"
#include "xbt/config.hpp"

/** Config Globals */
SG_BEGIN_DECL()

XBT_PUBLIC_DATA(xbt_cfg_t) simgrid_config;
XBT_PUBLIC_DATA(int) _sg_cfg_init_status;
XBT_PUBLIC_DATA(int) _sg_cfg_exit_asap;

XBT_PUBLIC(void) sg_config_init(int *argc, char **argv);
XBT_PUBLIC(void) sg_config_finalize();

SG_END_DECL()

#endif
