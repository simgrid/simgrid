/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_RoutedGraph.hpp"
#include "xbt/swag.h"


#ifndef SURF_ROUTING_VIVALDI_HPP_
#define SURF_ROUTING_VIVALDI_HPP_

/* ************************************************** */
/* **************  Vivaldi ROUTING   **************** */
XBT_PRIVATE AS_t model_vivaldi_create(void);      /* create structures for vivaldi routing model */

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/
class XBT_PRIVATE AsVivaldi;

class AsVivaldi: public AsRoutedGraph {
public:
  AsVivaldi(const char *name);
  ~AsVivaldi() {};

  void getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t into, double *latency) override;
};

}
}

#endif /* SURF_ROUTING_VIVALDI_HPP_ */
