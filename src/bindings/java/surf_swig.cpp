/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// Avoid ambiguity between boost and std placeholders
// (the std placeholders are imported through boost::signals2):
#ifndef BOOST_BIND_NO_PLACEHOLDERS
  #define BOOST_BIND_NO_PLACEHOLDERS
#endif

#include <functional>

#include <boost/lambda/bind.hpp>
#include "src/surf/surf_interface.hpp"
#include "surf_swig.hpp"
#include "src/simix/smx_private.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;

double getClock() {
  return surf_get_clock();
}

void clean() {
  SIMIX_clean();
}

simgrid::surf::CpuModel *getCpuModel(){
  return surf_cpu_model_pm;
}

simgrid::surf::CpuModel *java_cpu_model;
static void java_cpu_model_init_preparse() {
  surf_cpu_model_pm = java_cpu_model;
  xbt_dynar_push(all_existing_models, &java_cpu_model);
}

void setCpuModel(simgrid::surf::CpuModel *cpuModel){
  java_cpu_model = cpuModel;
  surf_cpu_model_init_preparse = java_cpu_model_init_preparse;
}

void setCpu(char *name, simgrid::surf::Cpu *cpu) {
	// No-op here for compatibility with previous versions
}

LinkDynar getRoute(char *srcName, char *dstName) {
  simgrid::surf::RoutingEdge *src = sg_host_edge(sg_host_by_name(srcName));
  simgrid::surf::RoutingEdge *dst = sg_host_edge(sg_host_by_name(dstName));
  xbt_assert(src,"Cannot get the route from a NULL source");
  xbt_assert(dst,"Cannot get the route to a NULL destination");
  xbt_dynar_t route = xbt_dynar_new(sizeof(simgrid::surf::RoutingEdge*), NULL);
  routing_platf->getRouteAndLatency(src, dst, &route, NULL);
  return route;
}

void Plugin::activateCpuCreatedCallback()
{
  surf_callback_connect(simgrid::surf::cpuCreatedCallbacks,
    std::bind(&Plugin::cpuCreatedCallback, this, _1));
}

void Plugin::activateCpuDestructedCallback()
{
  surf_callback_connect(simgrid::surf::cpuDestructedCallbacks,
    std::bind(&Plugin::cpuDestructedCallback, this, _1));
}

void Plugin::activateCpuStateChangedCallback()
{
  surf_callback_connect(simgrid::surf::cpuStateChangedCallbacks,
    std::bind(&Plugin::cpuStateChangedCallback, this, _1, _2, _3));
}

void Plugin::activateCpuActionStateChangedCallback()
{
  surf_callback_connect(simgrid::surf::cpuActionStateChangedCallbacks,
    std::bind(&Plugin::cpuActionStateChangedCallback, this, _1, _2, _3));
}


void Plugin::activateLinkCreatedCallback()
{
  surf_callback_connect(simgrid::surf::networkLinkCreatedCallbacks,
    std::bind(&Plugin::networkLinkCreatedCallback, this, _1));
}

void Plugin::activateLinkDestructedCallback()
{
  surf_callback_connect(simgrid::surf::networkLinkDestructedCallbacks,
    std::bind(&Plugin::networkLinkDestructedCallback, this, _1));
}

void Plugin::activateLinkStateChangedCallback()
{
  surf_callback_connect(simgrid::surf::networkLinkStateChangedCallbacks,
    std::bind(&Plugin::networkLinkStateChangedCallback, this, _1, _2, _3));
}

void Plugin::activateNetworkActionStateChangedCallback()
{
  surf_callback_connect(simgrid::surf::networkActionStateChangedCallbacks,
    std::bind(&Plugin::networkActionStateChangedCallback, this, _1, _2, _3));
}

void Plugin::activateNetworkCommunicateCallback()
{
  surf_callback_connect(simgrid::surf::networkCommunicateCallbacks,
    std::bind(&Plugin::networkCommunicateCallback, this, _1, _2, _3, _4, _5));
}



