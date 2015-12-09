/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include "network_interface.hpp"
#include "simgrid/sg_config.h"

#ifndef NETWORK_INTERFACE_CPP_
#define NETWORK_INTERFACE_CPP_

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network, surf,
                                "Logging specific to the SURF network module");

/*********
 * C API *
 *********/
SG_BEGIN_DECL()
const char* sg_link_name(Link *link) {
  return link->getName();
}
Link * sg_link_by_name(const char* name) {
  return Link::byName(name);
}

int sg_link_is_shared(Link *link){
  return link->sharingPolicy();
}
double sg_link_bandwidth(Link *link){
  return link->getBandwidth();
}
double sg_link_latency(Link *link){
  return link->getLatency();
}
void* sg_link_data(Link *link) {
	return link->getData();
}
void sg_link_data_set(Link *link,void *data) {
	link->setData(data);
}
int sg_link_amount(void) {
	return Link::linksAmount();
}
Link** sg_link_list(void) {
	return Link::linksList();
}
void sg_link_exit(void) {
	Link::linksExit();
}
SG_END_DECL()
/*****************
 * List of links *
 *****************/

boost::unordered_map<std::string,Link *> *Link::links = new boost::unordered_map<std::string,Link *>();
Link *Link::byName(const char* name) {
	  Link * res = NULL;
	  try {
		  res = links->at(name);
	  } catch (std::out_of_range& e) {}

	  return res;
}
/** @brief Returns the amount of links in the platform */
int Link::linksAmount() {
	  return links->size();
}
/** @brief Returns a list of all existing links */
Link **Link::linksList() {
	  Link **res = xbt_new(Link*, (int)links->size());
	  int i=0;
	  for (auto kv : *links) {
		  res[i++] = kv.second;
	  }
	  return res;
}
/** @brief destructor of the static data */
void Link::linksExit() {
	for (auto kv : *links)
		delete (kv.second);
	delete links;
}
/*************
 * Callbacks *
 *************/

surf_callback(void, Link*) networkLinkCreatedCallbacks;
surf_callback(void, Link*) networkLinkDestructedCallbacks;
surf_callback(void, Link*, e_surf_resource_state_t, e_surf_resource_state_t) networkLinkStateChangedCallbacks;
surf_callback(void, NetworkAction*, e_surf_action_state_t, e_surf_action_state_t) networkActionStateChangedCallbacks;
surf_callback(void, NetworkAction*, RoutingEdge *src, RoutingEdge *dst, double size, double rate) networkCommunicateCallbacks;

void netlink_parse_init(sg_platf_link_cbarg_t link){
  if (link->policy == SURF_LINK_FULLDUPLEX) {
    char *link_id;
    link_id = bprintf("%s_UP", link->id);
    surf_network_model->createLink(link_id,
                      link->bandwidth,
                      link->bandwidth_trace,
                      link->latency,
                      link->latency_trace,
                      link->state,
                      link->state_trace, link->policy, link->properties);
    xbt_free(link_id);
    link_id = bprintf("%s_DOWN", link->id);
    surf_network_model->createLink(link_id,
                      link->bandwidth,
                      link->bandwidth_trace,
                      link->latency,
                      link->latency_trace,
                      link->state,
                      link->state_trace, link->policy, link->properties);
    xbt_free(link_id);
  } else {
	  surf_network_model->createLink(link->id,
			  link->bandwidth,
			  link->bandwidth_trace,
			  link->latency,
			  link->latency_trace,
			  link->state,
			  link->state_trace, link->policy, link->properties);
  }
}

void net_add_traces(){
  surf_network_model->addTraces();
}

/*********
 * Model *
 *********/

NetworkModel *surf_network_model = NULL;

double NetworkModel::latencyFactor(double /*size*/) {
  return sg_latency_factor;
}

double NetworkModel::bandwidthFactor(double /*size*/) {
  return sg_bandwidth_factor;
}

double NetworkModel::bandwidthConstraint(double rate, double /*bound*/, double /*size*/) {
  return rate;
}

double NetworkModel::shareResourcesFull(double now)
{
  NetworkAction *action = NULL;
  ActionList *runningActions = surf_network_model->getRunningActionSet();
  double minRes;

  minRes = shareResourcesMaxMin(runningActions, surf_network_model->p_maxminSystem, surf_network_model->f_networkSolve);

  for(ActionList::iterator it(runningActions->begin()), itend(runningActions->end())
       ; it != itend ; ++it) {
      action = static_cast<NetworkAction*>(&*it);
#ifdef HAVE_LATENCY_BOUND_TRACKING
    if (lmm_is_variable_limited_by_latency(action->getVariable())) {
      action->m_latencyLimited = 1;
    } else {
      action->m_latencyLimited = 0;
    }
#endif
    if (action->m_latency > 0) {
      minRes = (minRes < 0) ? action->m_latency : std::min(minRes, action->m_latency);
    }
  }

  XBT_DEBUG("Min of share resources %f", minRes);

  return minRes;
}

/************
 * Resource *
 ************/

Link::Link(NetworkModel *model, const char *name, xbt_dict_t props)
: Resource(model, name, props)
{
  links->insert({name, this});

  XBT_DEBUG("Create link '%s'",name);
}

Link::Link(NetworkModel *model, const char *name, xbt_dict_t props,
		                 lmm_constraint_t constraint,
	                     tmgr_history_t history,
	                     tmgr_trace_t state_trace)
: Resource(model, name, props, constraint)
{
  if (state_trace)
    p_stateEvent = tmgr_history_add_trace(history, state_trace, 0.0, 0, this);

  links->insert({name, this});
  XBT_DEBUG("Create link '%s'",name);

}

Link::~Link()
{
  surf_callback_emit(networkLinkDestructedCallbacks, this);
}

bool Link::isUsed()
{
  return lmm_constraint_used(getModel()->getMaxminSystem(), getConstraint());
}

double Link::getLatency()
{
  return m_latCurrent;
}

double Link::getBandwidth()
{
  return p_speed.peak * p_speed.scale;
}

int Link::sharingPolicy()
{
  return lmm_constraint_sharing_policy(getConstraint());
}

void Link::setState(e_surf_resource_state_t state){
  e_surf_resource_state_t old = Resource::getState();
  Resource::setState(state);
  surf_callback_emit(networkLinkStateChangedCallbacks, this, old, state);
}



/**********
 * Action *
 **********/

void NetworkAction::setState(e_surf_action_state_t state){
  e_surf_action_state_t old = getState();
  Action::setState(state);
  surf_callback_emit(networkActionStateChangedCallbacks, this, old, state);
}

#endif /* NETWORK_INTERFACE_CPP_ */
