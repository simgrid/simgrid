/* a fast and simple context switching library                              */

/* Copyright (c) 2009-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/context/Context.hpp"

#define SIMIX_H_NO_DEPRECATED_WARNING // avoid deprecation warning on include (remove with XBT_ATTRIB_DEPRECATED_v333)
#include <simgrid/simix.h>

int SIMIX_context_is_parallel() // XBT_ATTRIB_DEPRECATED_v333
{
  return simgrid::kernel::context::is_parallel();
}

int SIMIX_context_get_nthreads() // XBT_ATTRIB_DEPRECATED_v333
{
  return simgrid::kernel::context::get_nthreads();
}

void SIMIX_context_set_nthreads(int nb_threads) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::kernel::context::set_nthreads(nb_threads);
}

e_xbt_parmap_mode_t SIMIX_context_get_parallel_mode() // XBT_ATTRIB_DEPRECATED_v333
{
  return simgrid::kernel::context::get_parallel_mode();
}

void SIMIX_context_set_parallel_mode(e_xbt_parmap_mode_t mode) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::kernel::context::set_parallel_mode(mode);
}
