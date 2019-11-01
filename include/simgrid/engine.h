/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_ENGINE_H_
#define INCLUDE_SIMGRID_ENGINE_H_

#include <simgrid/forward.h>

/* C interface */
SG_BEGIN_DECL
XBT_PUBLIC void simgrid_init(int* argc, char** argv);
XBT_PUBLIC void simgrid_load_platform(const char* filename);
XBT_PUBLIC void simgrid_load_deployment(const char* filename);
XBT_PUBLIC void simgrid_run();
XBT_PUBLIC void simgrid_register_function(const char* name, int (*code)(int, char**));
XBT_PUBLIC void simgrid_register_default(int (*code)(int, char**));
XBT_PUBLIC double simgrid_get_clock();

XBT_PUBLIC void sg_config_continue_after_help();
SG_END_DECL

#endif /* INCLUDE_SIMGRID_ENGINE_H_ */
