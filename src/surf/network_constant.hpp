/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETWORK_CONSTANT_HPP_
#define NETWORK_CONSTANT_HPP_

#include <xbt/base.h>

#include "network_interface.hpp"

/***********
 * Classes *
 ***********/
class XBT_PRIVATE NetworkConstantModel;
class XBT_PRIVATE NetworkConstantAction;

/*********
 * Model *
 *********/
class NetworkConstantModel : public NetworkModel {
public:
  NetworkConstantModel()  : NetworkModel() { };
  ~NetworkConstantModel() { }

  Action *communicate(RoutingEdge *src, RoutingEdge *dst, double size, double rate);
  double shareResources(double now);
  void updateActionsState(double now, double delta);
  bool shareResourcesIsIdempotent() {return true;}

  Link* createLink(const char *name,
		           double bw_initial,
				   tmgr_trace_t bw_trace,
				   double lat_initial,
				   tmgr_trace_t lat_trace,
				   e_surf_resource_state_t state_initial,
				   tmgr_trace_t state_trace,
				   e_surf_link_sharing_policy_t policy,
				   xbt_dict_t properties)                  { DIE_IMPOSSIBLE; }
  void addTraces()                                         { DIE_IMPOSSIBLE; }
  xbt_dynar_t getRoute(RoutingEdge *src, RoutingEdge *dst) { DIE_IMPOSSIBLE; }
};

/**********
 * Action *
 **********/
class NetworkConstantAction : public NetworkAction {
public:
  NetworkConstantAction(NetworkConstantModel *model_, double size, double latency)
  : NetworkAction(model_, size, false)
  , m_latInit(latency)
  {
	m_latency = latency;
	if (m_latency <= 0.0) {
	  p_stateSet = getModel()->getDoneActionSet();
	  p_stateSet->push_back(*this);
	}
	p_variable = NULL;
  };
  int unref();
  void cancel();
  void setCategory(const char *category);
  void suspend();
  void resume();
  bool isSuspended();
  double m_latInit;
  int m_suspended;
};

#endif /* NETWORK_CONSTANT_HPP_ */
