/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_VIVALDI_HPP_
#define SURF_ROUTING_VIVALDI_HPP_

#include <simgrid/kernel/routing/StarZone.hpp>
#include <xbt/Extendable.hpp>

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
 *  So, to go from a host A to a host B, the following links would be used:
 *  <tt>private(A)_UP, private(B)_DOWN</tt>, with the additional latency computed above.
 *  The bandwidth of the UP and DOWN links is not symmetric (in contrary to usual SimGrid
 *  links), but naturally correspond to the values provided when the peer was created.
 *  More information in the relevant section of the XML reference guide: @ref pf_peer.
 *
 *  You can find some Coordinate-based platforms from the OptorSim project, as well as a
 *  script to turn them into SimGrid platforms in examples/platforms/syscoord.
 *
 *  Such Network Coordinate systems were shown to provide rather good latency estimations
 *  in a compact way. Other systems, such as
 *  <a href="https://en.wikipedia.org/wiki/Phoenix_network_coordinates"Phoenix network coordinates</a>
 *  were shown superior to the Vivaldi system and could be also implemented in SimGrid.
 */

class XBT_PRIVATE VivaldiZone : public StarZone {
public:
  using StarZone::StarZone;
  void set_peer_link(NetPoint* netpoint, double bw_in, double bw_out);
  void get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* into, double* latency) override;
};

namespace vivaldi {
class XBT_PRIVATE Coords {
public:
  static xbt::Extension<NetPoint, Coords> EXTENSION_ID;
  explicit Coords(NetPoint* host, const std::string& str);

  std::vector<double> coords;
};
} // namespace vivaldi
} // namespace routing
} // namespace kernel
} // namespace simgrid

#endif /* SURF_ROUTING_VIVALDI_HPP_ */
