/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_VIVALDI_HPP_
#define SURF_ROUTING_VIVALDI_HPP_

#include "src/surf/AsRoutedGraph.hpp"

namespace simgrid {
namespace surf {

class XBT_PRIVATE AsVivaldi: public AsRoutedGraph {
public:
  AsVivaldi(const char *name);
  ~AsVivaldi() {};

  xbt_dynar_t getOneLinkRoutes() override {return NULL;};
  void getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t into, double *latency) override;
};

}
}

#endif /* SURF_ROUTING_VIVALDI_HPP_ */
