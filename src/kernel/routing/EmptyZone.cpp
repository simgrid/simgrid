/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/dict.h>
#include <xbt/graph.h>
#include <xbt/log.h>

#include "simgrid/kernel/routing/EmptyZone.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_none, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

EmptyZone::EmptyZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel)
    : NetZoneImpl(father, name, netmodel)
{
}

EmptyZone::~EmptyZone() = default;

void EmptyZone::get_graph(const s_xbt_graph_t* /*graph*/, std::map<std::string, xbt_node_t, std::less<>>* /*nodes*/,
                          std::map<std::string, xbt_edge_t, std::less<>>* /*edges*/)
{
  XBT_ERROR("No routing no graph");
}
} // namespace routing
} // namespace kernel
} // namespace simgrid
