/* Copyright (c) 2012-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_CONFIG_HPP
#define SIMGRID_CONFIG_HPP

#include "xbt/base.h"
#include "xbt/config.hpp"

/** Config Globals */

XBT_PUBLIC void sg_config_init(int* argc, char** argv);

/* 0: beginning of time (config cannot be changed yet)
 * 1: initialized: cfg_set created (config can now be changed)
 * 2: configured: command line parsed and config part of platform file was integrated also, platform construction
 * ongoing or done. (Config cannot be changed anymore!)
 */
XBT_PUBLIC bool sg_configuration_is_not_started(); // state == 0
XBT_PUBLIC bool sg_configuration_is_ongoing();     // state == 1
XBT_PUBLIC bool sg_configuration_is_done();        // state == 2
XBT_PUBLIC void sg_configuration_set_step(int step);

XBT_PUBLIC void sg_config_finalize();

#endif
