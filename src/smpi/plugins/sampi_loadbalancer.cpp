/* Copyright (c) 2018.      The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/load_balancer.h>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/smpi/replay.hpp>
#include <xbt/replay.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(plugin_load_balancer, surf, "Logging specific to the SMPI load balancing plugin");

namespace simgrid {
namespace smpi {
namespace plugin {


}
}
}

/** \ingroup plugin_loadbalancer
 * \brief Initializes the load balancer plugin
 * \details The load balancer plugin supports several AMPI load balancers that move ranks
 * around, based on their host's load.
 */
void sg_load_balancer_plugin_init()
{
}
