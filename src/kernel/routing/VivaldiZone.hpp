/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_VIVALDI_HPP_
#define SURF_ROUTING_VIVALDI_HPP_

#include "src/kernel/routing/ClusterZone.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

/* This extends cluster because each host has a private link */
class XBT_PRIVATE VivaldiZone : public ClusterZone {
public:
  explicit VivaldiZone(NetZone* father, const char* name);

  void setPeerLink(NetCard* netcard, double bw_in, double bw_out, double lat, const char* coord);
  void getLocalRoute(NetCard* src, NetCard* dst, sg_platf_route_cbarg_t into, double* latency) override;
};

namespace vivaldi {
class XBT_PRIVATE Coords {
public:
  static simgrid::xbt::Extension<NetCard, Coords> EXTENSION_ID;
  explicit Coords(NetCard* host, const char* str);
  virtual ~Coords();

  std::vector<double> coords;
};
}
}
}
} // namespace

#endif /* SURF_ROUTING_VIVALDI_HPP_ */
