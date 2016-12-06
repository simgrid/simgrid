/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_ROUTING_HPP_
#define NETWORK_ROUTING_HPP_

#include <xbt/base.h>
#include <xbt/signal.hpp>

#include "surf_interface.hpp"
#include "src/kernel/routing/AsImpl.hpp"

#include <float.h>
#include <vector>

SG_BEGIN_DECL()
XBT_PRIVATE xbt_node_t new_xbt_graph_node (xbt_graph_t graph, const char *name, xbt_dict_t nodes);
XBT_PRIVATE xbt_edge_t new_xbt_graph_edge (xbt_graph_t graph, xbt_node_t s, xbt_node_t d, xbt_dict_t edges);
SG_END_DECL()

namespace simgrid {
namespace kernel {
namespace routing {

  XBT_PUBLIC_DATA(simgrid::xbt::signal<void(s4u::As*)>) asCreatedCallbacks;

/***********
 * Classes *
 ***********/

class XBT_PRIVATE Onelink;
class RoutingPlatf;

class AsRoute {
public:
  explicit AsRoute(NetCard* gwSrc, NetCard* gwDst) : gw_src(gwSrc), gw_dst(gwDst) {}
  const NetCard* gw_src;
  const NetCard* gw_dst;
  std::vector<Link*> links;
};

/** @ingroup SURF_routing_interface
 * @brief Link of length 1, alongside with its source and destination. This is mainly useful in the ns3 bindings
 */
class Onelink {
public:
  Onelink(Link* link, NetCard* src, NetCard* dst) : src_(src), dst_(dst), link_(link){};
  NetCard* src_;
  NetCard* dst_;
  Link* link_;
};

}}}

#endif /* NETWORK_ROUTING_HPP_ */
