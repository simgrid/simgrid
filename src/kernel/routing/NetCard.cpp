/* Copyright (c) 2009-2011, 2013-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/NetCard.hpp"
#include "src/surf/surf_routing.hpp"
#include <simgrid/s4u/host.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_route);

namespace simgrid {
namespace kernel {
namespace routing {

simgrid::xbt::signal<void(NetCard*)> NetCard::onCreation;
}
}
} // namespace simgrid::kernel::routing

/** @brief Retrieve a netcard from its name
 *
 * Netcards are the thing that connect host or routers to the network
 */
simgrid::kernel::routing::NetCard* sg_netcard_by_name_or_null(const char* name)
{
  sg_host_t h                                = sg_host_by_name(name);
  simgrid::kernel::routing::NetCard* netcard = h == nullptr ? nullptr : h->pimpl_netcard;
  if (!netcard)
    netcard =
        static_cast<simgrid::kernel::routing::NetCard*>(xbt_lib_get_or_null(as_router_lib, name, ROUTING_ASR_LEVEL));
  return netcard;
}
