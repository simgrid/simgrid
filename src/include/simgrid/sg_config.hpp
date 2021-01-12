/* Copyright (c) 2012-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_CONFIG_HPP
#define SIMGRID_CONFIG_HPP

#include "xbt/config.hpp"

/** Config Globals */

XBT_PUBLIC_DATA int _sg_cfg_init_status;

XBT_PUBLIC void sg_config_init(int* argc, char** argv);
XBT_PUBLIC void sg_config_finalize();

#endif
