/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <boost/lambda/bind.hpp>
#include "src/surf/surf_interface.hpp"
#include "surf_swig.hpp"
#include "src/simix/smx_private.h"


double getClock() {
  return surf_get_clock();
}

void clean() {
  SIMIX_clean();
}

/*NetworkModel *getNetworkModel()
{
  return surf_network_model;
}*/

NetworkLinkDynar getRoute(char *srcName, char *dstName) {
  RoutingEdge *src = (RoutingEdge*)xbt_lib_get_or_null(host_lib, srcName, ROUTING_HOST_LEVEL);
  RoutingEdge *dst = (RoutingEdge*)xbt_lib_get_or_null(host_lib, dstName, ROUTING_HOST_LEVEL);
  if (src==NULL)
    xbt_die("TOTO");
  if (dst==NULL)
    xbt_die("TOTO");
  xbt_dynar_t route = xbt_dynar_new(sizeof(RoutingEdgePtr), NULL);
  routing_platf->getRouteAndLatency(src, dst, &route, NULL);
  return route;
}

void Plugin::activateCpuCreatedCallback(){
  surf_callback_connect(cpuCreatedCallbacks, boost::bind(&Plugin::cpuCreatedCallback, this, _1));
}

void Plugin::activateCpuDestructedCallback(){
  surf_callback_connect(cpuDestructedCallbacks, boost::bind(&Plugin::cpuDestructedCallback, this, _1));
}

void Plugin::activateCpuStateChangedCallback(){
  surf_callback_connect(cpuStateChangedCallbacks, boost::bind(&Plugin::cpuStateChangedCallback, this, _1, _2, _3));
}

void Plugin::activateCpuActionStateChangedCallback(){
  surf_callback_connect(cpuActionStateChangedCallbacks, boost::bind(&Plugin::cpuActionStateChangedCallback, this, _1, _2, _3));
}


void Plugin::activateNetworkLinkCreatedCallback(){
  surf_callback_connect(networkLinkCreatedCallbacks, boost::bind(&Plugin::networkLinkCreatedCallback, this, _1));
}

void Plugin::activateNetworkLinkDestructedCallback(){
  surf_callback_connect(networkLinkDestructedCallbacks, boost::bind(&Plugin::networkLinkDestructedCallback, this, _1));
}

void Plugin::activateNetworkLinkStateChangedCallback(){
  surf_callback_connect(networkLinkStateChangedCallbacks, boost::bind(&Plugin::networkLinkStateChangedCallback, this, _1, _2, _3));
}

void Plugin::activateNetworkActionStateChangedCallback(){
  surf_callback_connect(networkActionStateChangedCallbacks, boost::bind(&Plugin::networkActionStateChangedCallback, this, _1, _2, _3));
}

void Plugin::activateNetworkCommunicateCallback(){
  surf_callback_connect(networkCommunicateCallbacks, boost::bind(&Plugin::networkCommunicateCallback, this, _1, _2, _3, _4, _5));
}



