/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef KERNEL_ROUTING_BYPASSROUTE_HPP_
#define KERNEL_ROUTING_BYPASSROUTE_HPP_

#include <xbt/base.h>
#include <xbt/signal.hpp>

#include "src/kernel/routing/NetCard.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

class BypassRoute {
public:
  explicit BypassRoute(NetCard* gwSrc, NetCard* gwDst) : gw_src(gwSrc), gw_dst(gwDst) {}
  const NetCard* gw_src;
  const NetCard* gw_dst;
  std::vector<Link*> links;
};
}
}
}

#endif /* KERNEL_ROUTING_BYPASSROUTE_HPP_ */
