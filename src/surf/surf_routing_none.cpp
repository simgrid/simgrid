/* Copyright (c) 2009-2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/surf_routing_private.hpp"
#include "src/surf/surf_routing_none.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_none, surf, "Routing part of surf");

namespace simgrid {
namespace surf {
AsNone::AsNone(const char*name)
  : As(name)
{}
AsNone::~AsNone()
{}

xbt_dynar_t AsNone::getOneLinkRoutes() {
  return NULL;
}

void AsNone::getRouteAndLatency(NetCard * /*src*/, NetCard * /*dst*/,
                                sg_platf_route_cbarg_t /*res*/, double */*lat*/)
{}

void AsNone::getGraph(xbt_graph_t /*graph*/, xbt_dict_t /*nodes*/, xbt_dict_t /*edges*/)
{
  XBT_INFO("No routing no graph");
}
}
}
