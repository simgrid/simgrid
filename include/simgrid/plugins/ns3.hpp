/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_LOAD_H_
#define SIMGRID_PLUGINS_LOAD_H_

#include <ns3/node.h>
#include <ns3/ptr.h>
#include <simgrid/config.h>
#include <simgrid/forward.h>
#include <xbt/base.h>

namespace simgrid {
/** Returns the ns3 node from a simgrid host */
XBT_PUBLIC ns3::Ptr<ns3::Node> get_ns3node_from_sghost(simgrid::s4u::Host* host);
}; // namespace simgrid

#endif
