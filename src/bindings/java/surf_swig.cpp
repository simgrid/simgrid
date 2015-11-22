/* Copyright (c) 2014-2015. The SimGrid Team.
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

CpuModel *getCpuModel(){
  return surf_cpu_model_pm;
}

CpuModel *java_cpu_model;
static void java_cpu_model_init_preparse() {
  surf_cpu_model_pm = java_cpu_model;
  xbt_dynar_push(all_existing_models, &java_cpu_model);
  xbt_dynar_push(model_list_invoke, &java_cpu_model);
  sg_platf_host_add_cb(cpu_parse_init);
}

void setCpuModel(CpuModel *cpuModel){
  java_cpu_model = cpuModel;
  surf_cpu_model_init_preparse = java_cpu_model_init_preparse;
}

void setCpu(char *name, Cpu *cpu) {
	sg_host_surfcpu_set(sg_host_by_name(name), cpu);
}

LinkDynar getRoute(char *srcName, char *dstName) {
  RoutingEdge *src = sg_host_edge(sg_host_by_name(srcName));
  RoutingEdge *dst = sg_host_edge(sg_host_by_name(dstName));
  xbt_assert(src,"Cannot get the route from a NULL source");
  xbt_assert(dst,"Cannot get the route to a NULL destination");
  xbt_dynar_t route = xbt_dynar_new(sizeof(RoutingEdge*), NULL);
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


void Plugin::activateLinkCreatedCallback(){
  surf_callback_connect(networkLinkCreatedCallbacks, boost::bind(&Plugin::networkLinkCreatedCallback, this, _1));
}

void Plugin::activateLinkDestructedCallback(){
  surf_callback_connect(networkLinkDestructedCallbacks, boost::bind(&Plugin::networkLinkDestructedCallback, this, _1));
}

void Plugin::activateLinkStateChangedCallback(){
  surf_callback_connect(networkLinkStateChangedCallbacks, boost::bind(&Plugin::networkLinkStateChangedCallback, this, _1, _2, _3));
}

void Plugin::activateNetworkActionStateChangedCallback(){
  surf_callback_connect(networkActionStateChangedCallbacks, boost::bind(&Plugin::networkActionStateChangedCallback, this, _1, _2, _3));
}

void Plugin::activateNetworkCommunicateCallback(){
  surf_callback_connect(networkCommunicateCallbacks, boost::bind(&Plugin::networkCommunicateCallback, this, _1, _2, _3, _4, _5));
}



