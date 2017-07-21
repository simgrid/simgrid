/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/NetPoint.hpp"

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

simgrid::xbt::signal<void(NetPoint*)> NetPoint::onCreation;

NetPoint::NetPoint(std::string name, NetPoint::Type componentType, NetZoneImpl* netzone_p)
    : name_(name), componentType_(componentType), netzone_(netzone_p)
{
  if (netzone_p != nullptr)
    id_ = netzone_p->addComponent(this);
  simgrid::s4u::Engine::getInstance()->netpointRegister(this);
  simgrid::kernel::routing::NetPoint::onCreation(this);
}
}
}
} // namespace simgrid::kernel::routing

/** @brief Retrieve a netpoint from its name
 *
 * Netpoints denote the location of host or routers in the network, to compute routes
 */
simgrid::kernel::routing::NetPoint* sg_netpoint_by_name_or_null(const char* name)
{
  return simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name);
}
