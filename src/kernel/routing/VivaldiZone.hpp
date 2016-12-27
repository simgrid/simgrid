/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_VIVALDI_HPP_
#define SURF_ROUTING_VIVALDI_HPP_

#include "src/kernel/routing/ClusterZone.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone modeling peers connected to the cloud through a private link
 *
 *  This netzone model is particularly well adapted to Peer-to-Peer and Clouds platforms:
 *  each component is connected to the cloud through a private link of which the upload
 *  and download rate may be asymmetric.
 *
 *  The network core (between the private links) is assumed to be over-sized so only the
 *  latency is taken into account. Instead of a matrix of latencies that would become too
 *  large when the amount of peers grows, Vivaldi netzones give a coordinate to each peer
 *  and compute the latency between host A=(xA,yA,zA) and host B=(xB,yB,zB) as follows:
 *
 *   latency = sqrt( (xA-xB)² + (yA-yB)² ) + zA + zB
 *
 *  The resulting value is assumed to be in milliseconds.
 *
 *  So, to go from an host A to an host B, the following links would be used:
 *  <tt>private(A)_UP, private(B)_DOWN</tt>, with the additional latency computed above.
 *
 *  Such Network Coordinate systems were shown to provide rather good latency estimations
 *  in a compact way. Other systems, such as
 *  <a href="https://en.wikipedia.org/wiki/Phoenix_network_coordinates"Phoenix network coordinates</a>
 *  were shown superior to the Vivaldi system and could be also implemented in SimGrid.
 *
 *
 *  @todo: the third dimension of the coordinates could be dropped and integrated in the peer private links.
 *
 *  @todo: we should provide a script to compute the coordinates from a matrix of latency measurements,
 *  according to the corresponding publications.
 */

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
