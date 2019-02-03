/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/engine.h>
#include <simgrid/simix.h>

void SIMIX_create_environment(const char* file) // XBT_ATTRIB_DEPRECATED_v324
{
  simgrid_load_platform(file);
}
