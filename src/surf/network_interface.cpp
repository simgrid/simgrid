/*
 * network_interface.cpp
 *
 *  Created on: Nov 29, 2013
 *      Author: bedaride
 */
#include "network_interface.hpp"
#include "simgrid/sg_config.h"

#ifndef NETWORK_INTERFACE_CPP_
#define NETWORK_INTERFACE_CPP_

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network, surf,
                                "Logging specific to the SURF network module");

NetworkModelPtr surf_network_model = NULL;

xbt_dynar_t NetworkModel::getRoute(RoutingEdgePtr src, RoutingEdgePtr dst)
{
  xbt_dynar_t route = NULL;
  routing_platf->getRouteAndLatency(src, dst, &route, NULL);
  return route;
}

double NetworkModel::latencyFactor(double /*size*/) {
  return sg_latency_factor;
}

double NetworkModel::bandwidthFactor(double /*size*/) {
  return sg_bandwidth_factor;
}

double NetworkModel::bandwidthConstraint(double rate, double /*bound*/, double /*size*/) {
  return rate;
}

NetworkLinkLmm::NetworkLinkLmm(lmm_constraint_t constraint,
	                           tmgr_history_t history,
	                           tmgr_trace_t state_trace)
: ResourceLmm(constraint)
{
  if (state_trace)
    p_stateEvent = tmgr_history_add_trace(history, state_trace, 0.0, 0, static_cast<ResourcePtr>(this));
}

bool NetworkLinkLmm::isUsed()
{
  return lmm_constraint_used(getModel()->getMaxminSystem(), constraint());
}

double NetworkLink::getLatency()
{
  return m_latCurrent;
}

double NetworkLinkLmm::getBandwidth()
{
  return p_power.peak * p_power.scale;
}

bool NetworkLinkLmm::isShared()
{
  return lmm_constraint_is_shared(constraint());
}

#endif /* NETWORK_INTERFACE_CPP_ */
