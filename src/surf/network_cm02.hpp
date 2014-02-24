/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_interface.hpp"
#include "xbt/fifo.h"
#include "xbt/graph.h"

#ifndef SURF_NETWORK_CM02_HPP_
#define SURF_NETWORK_CM02_HPP_

/***********
 * Classes *
 ***********/
class NetworkCm02Model;
typedef NetworkCm02Model *NetworkCm02ModelPtr;

class NetworkCm02Link;
typedef NetworkCm02Link *NetworkCm02LinkPtr;

class NetworkCm02Action;
typedef NetworkCm02Action *NetworkCm02ActionPtr;

/*********
 * Tools *
 *********/

void net_define_callbacks(void);

/*********
 * Model *
 *********/
class NetworkCm02Model : public NetworkModel {
private:
  void initialize();
public:
  NetworkCm02Model(int /*i*/) : NetworkModel("network") {
	f_networkSolve = lmm_solve;
	m_haveGap = false;
  };//FIXME: add network clean interface
  NetworkCm02Model(const char *name) : NetworkModel(name) {
    this->initialize();
  }
  NetworkCm02Model() : NetworkModel("network") {
    this->initialize();
  }
  ~NetworkCm02Model() {
  }
  NetworkLinkPtr createResource(const char *name,
                                   double bw_initial,
                                   tmgr_trace_t bw_trace,
                                   double lat_initial,
                                   tmgr_trace_t lat_trace,
                                   e_surf_resource_state_t state_initial,
                                   tmgr_trace_t state_trace,
                                   e_surf_link_sharing_policy_t policy,
                                   xbt_dict_t properties);
  void updateActionsStateLazy(double now, double delta);
  ActionPtr communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
		                           double size, double rate);
};

/************
 * Resource *
 ************/

class NetworkCm02Link : public NetworkLink {
public:
  NetworkCm02Link(NetworkCm02ModelPtr model, const char *name, xbt_dict_t props,
	                           lmm_system_t system,
	                           double constraint_value,
	                           tmgr_history_t history,
	                           e_surf_resource_state_t state_init,
	                           tmgr_trace_t state_trace,
	                           double metric_peak,
	                           tmgr_trace_t metric_trace,
	                           double lat_initial,
	                           tmgr_trace_t lat_trace,
                               e_surf_link_sharing_policy_t policy);
  void updateState(tmgr_trace_event_t event_type, double value, double date);
};


/**********
 * Action *
 **********/

class NetworkCm02Action : public NetworkAction {
  friend ActionPtr NetworkCm02Model::communicate(RoutingEdgePtr src, RoutingEdgePtr dst, double size, double rate);

public:
  NetworkCm02Action(ModelPtr model, double cost, bool failed)
 : NetworkAction(model, cost, failed) {};
  void updateRemainingLazy(double now);
  void recycle();
};

#endif /* SURF_NETWORK_CM02_HPP_ */
