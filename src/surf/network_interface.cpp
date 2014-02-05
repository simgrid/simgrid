/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_interface.hpp"
#include "simgrid/sg_config.h"

#ifndef NETWORK_INTERFACE_CPP_
#define NETWORK_INTERFACE_CPP_

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network, surf,
                                "Logging specific to the SURF network module");

/*************
 * Callbacks *
 *************/

surf_callback(void, NetworkLinkPtr) networkLinkCreatedCallbacks;
surf_callback(void, NetworkLinkPtr) networkLinkDestructedCallbacks;
surf_callback(void, NetworkLinkPtr) networkLinkStateChangedCallbacks;
surf_callback(void, NetworkActionPtr) networkActionStateChangedCallbacks;

/*********
 * Model *
 *********/

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

/************
 * Resource *
 ************/

NetworkLink::NetworkLink(NetworkModelPtr model, const char *name, xbt_dict_t props)
: Resource(model, name, props)
, p_latEvent(NULL)
{
  surf_callback_emit(networkLinkCreatedCallbacks, this);
}

NetworkLink::NetworkLink(NetworkModelPtr model, const char *name, xbt_dict_t props,
		                 lmm_constraint_t constraint,
	                     tmgr_history_t history,
	                     tmgr_trace_t state_trace)
: Resource(model, name, props, constraint),
  p_latEvent(NULL)
{
  surf_callback_emit(networkLinkCreatedCallbacks, this);
  if (state_trace)
    p_stateEvent = tmgr_history_add_trace(history, state_trace, 0.0, 0, static_cast<ResourcePtr>(this));
}

NetworkLink::~NetworkLink()
{
  surf_callback_emit(networkLinkDestructedCallbacks, this);
}

bool NetworkLink::isUsed()
{
  return lmm_constraint_used(getModel()->getMaxminSystem(), getConstraint());
}

double NetworkLink::getLatency()
{
  return m_latCurrent;
}

double NetworkLink::getBandwidth()
{
  return p_power.peak * p_power.scale;
}

bool NetworkLink::isShared()
{
  return lmm_constraint_is_shared(getConstraint());
}

void NetworkLink::setState(e_surf_resource_state_t state){
  Resource::setState(state);
  surf_callback_emit(networkLinkStateChangedCallbacks, this);
}

/**********
 * Action *
 **********/

void NetworkAction::setState(e_surf_action_state_t state){
  Action::setState(state);
  surf_callback_emit(networkActionStateChangedCallbacks, this);
}

#endif /* NETWORK_INTERFACE_CPP_ */
