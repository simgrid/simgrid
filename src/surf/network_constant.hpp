/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_cm02.hpp"

#ifndef NETWORK_CONSTANT_HPP_
#define NETWORK_CONSTANT_HPP_

/***********
 * Classes *
 ***********/
class NetworkConstantModel;
class NetworkConstantAction;

/*********
 * Model *
 *********/
class NetworkConstantModel : public NetworkCm02Model {
public:
  NetworkConstantModel()
  : NetworkCm02Model("constant time network")
  {
    p_updateMechanism = UM_UNDEFINED;
  };
  double shareResources(double now);
  void updateActionsState(double now, double delta);
  Action *communicate(RoutingEdge *src, RoutingEdge *dst,
		                           double size, double rate);
  void gapRemove(Action *action);
};

/************
 * Resource *
 ************/
class NetworkConstantLink : public NetworkCm02Link {
public:
  NetworkConstantLink(NetworkCm02Model *model, const char* name, xbt_dict_t properties);
  bool isUsed();
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  double getBandwidth();
  double getLatency();
  bool isShared();
};

/**********
 * Action *
 **********/
class NetworkConstantAction : public NetworkCm02Action {
public:
  NetworkConstantAction(NetworkConstantModel *model_, double size, double latency)
  : NetworkCm02Action(model_, size, false)
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
