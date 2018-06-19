/* Copyright (c) 2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_ENGINE_H_
#define INCLUDE_SIMGRID_ENGINE_H_

#include <simgrid/forward.h>

/* C interface */
SG_BEGIN_DECL()
XBT_PUBLIC void sg_engine_load_platform(const char* filename);
XBT_PUBLIC void sg_engine_load_deployment(const char* filename);
XBT_PUBLIC void sg_engine_run();
XBT_PUBLIC void sg_engine_register_function(const char* name, int (*code)(int, char**));
XBT_PUBLIC void sg_engine_register_default(int (*code)(int, char**));
XBT_PUBLIC double sg_engine_get_clock();
SG_END_DECL()

#endif /* INCLUDE_SIMGRID_ENGINE_H_ */
