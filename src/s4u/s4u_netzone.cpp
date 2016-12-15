/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/s4u/host.hpp"
#include "src/kernel/routing/NetCard.hpp"
#include "src/surf/network_interface.hpp" // Link FIXME: move to proper header
#include "src/surf/surf_routing.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_netzone, "S4U Networking Zones");

namespace simgrid {
namespace s4u {

simgrid::xbt::signal<void(bool symmetrical, kernel::routing::NetCard* src, kernel::routing::NetCard* dst,
                          kernel::routing::NetCard* gw_src, kernel::routing::NetCard* gw_dst,
                          std::vector<Link*>* link_list)>
    NetZone::onRouteCreation;

NetZone::NetZone(NetZone* father, const char* name) : father_(father), name_(xbt_strdup(name))
{
}
void NetZone::seal()
{
  sealed_ = true;
}
NetZone::~NetZone()
{
  xbt_dict_cursor_t cursor = nullptr;
  char* key;
  NetZone_t elem;
  xbt_dict_foreach (children_, cursor, key, elem) {
    delete static_cast<NetZone*>(elem);
  }

  xbt_dict_free(&children_);
  xbt_free(name_);
}

xbt_dict_t NetZone::children()
{
  return children_;
}
char* NetZone::name()
{
  return name_;
}
NetZone* NetZone::father()
{
  return father_;
}

xbt_dynar_t NetZone::hosts()
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(sg_host_t), nullptr);

  for (auto card : vertices_) {
    s4u::Host* host = simgrid::s4u::Host::by_name_or_null(card->name());
    if (host != nullptr)
      xbt_dynar_push(res, &host);
  }
  return res;
}

int NetZone::addComponent(kernel::routing::NetCard* elm)
{
  vertices_.push_back(elm);
  return vertices_.size() - 1; // The rank of the newly created object
}

void NetZone::addRoute(sg_platf_route_cbarg_t /*route*/)
{
  xbt_die("NetZone '%s' does not accept new routes (wrong class).", name_);
}
}
}; // namespace simgrid::s4u
