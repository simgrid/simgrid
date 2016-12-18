/* Copyright (c) 2009-2011, 2013-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/NetCard.hpp"
#include "simgrid/s4u/engine.hpp"
#include "simgrid/s4u/host.hpp"
#include "surf/surf_routing.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

simgrid::xbt::signal<void(NetCard*)> NetCard::onCreation;

NetCard::NetCard(std::string name, NetCard::Type componentType, NetZoneImpl* netzone_p)
    : name_(name), componentType_(componentType), netzone_(netzone_p)
{
  if (netzone_p != nullptr)
    id_ = netzone_p->addComponent(this);
  xbt_dict_set(netcards_dict, name.c_str(), static_cast<void*>(this), nullptr);
  simgrid::kernel::routing::NetCard::onCreation(this);
}
}
}
} // namespace simgrid::kernel::routing

/** @brief Retrieve a netcard from its name
 *
 * Netcards are the thing that connect host or routers to the network
 */
simgrid::kernel::routing::NetCard* sg_netcard_by_name_or_null(const char* name)
{
  return simgrid::s4u::Engine::instance()->netcardByNameOrNull(name);
}
