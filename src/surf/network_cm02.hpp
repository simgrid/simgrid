/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_CM02_HPP_
#define SURF_NETWORK_CM02_HPP_

#include <xbt/base.h>

#include "network_interface.hpp"
#include "xbt/fifo.h"
#include "xbt/graph.h"

/***********
 * Classes *
 ***********/
class XBT_PRIVATE NetworkCm02Model;
class XBT_PRIVATE NetworkCm02Action;
class XBT_PRIVATE NetworkSmpiModel;

/*********
 * Tools *
 *********/

XBT_PRIVATE void net_define_callbacks(void);

/*********
 * Model *
 *********/
class NetworkCm02Model : public NetworkModel {
private:
  void initialize();
public:
  NetworkCm02Model(int /*i*/) : NetworkModel() {};
  NetworkCm02Model();
  ~NetworkCm02Model() { }
  Link* createLink(const char *name,
		  double bw_initial,
		  tmgr_trace_t bw_trace,
		  double lat_initial,
		  tmgr_trace_t lat_trace,
		  e_surf_resource_state_t state_initial,
		  tmgr_trace_t state_trace,
		  e_surf_link_sharing_policy_t policy,
		  xbt_dict_t properties) override;
  void addTraces();
  void updateActionsStateLazy(double now, double delta);
  void updateActionsStateFull(double now, double delta);
  Action *communicate(RoutingEdge *src, RoutingEdge *dst,
		                           double size, double rate);
  bool shareResourcesIsIdempotent() {return true;}
  virtual void gapAppend(double /*size*/, const Link* /*link*/, NetworkAction * /*action*/) {};
  bool m_haveGap = false;
};

/************
 * Resource *
 ************/

class NetworkCm02Link : public Link {
public:
  NetworkCm02Link(NetworkCm02Model *model, const char *name, xbt_dict_t props,
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
  void updateBandwidth(double value, double date=surf_get_clock());
  void updateLatency(double value, double date=surf_get_clock());
  virtual void gapAppend(double /*size*/, const Link* /*link*/, NetworkAction * /*action*/) {};


};


/**********
 * Action *
 **********/
class NetworkCm02Action : public NetworkAction {
  friend Action *NetworkCm02Model::communicate(RoutingEdge *src, RoutingEdge *dst, double size, double rate);
  friend NetworkSmpiModel;

public:
  NetworkCm02Action(Model *model, double cost, bool failed)
     : NetworkAction(model, cost, failed) {};
  void updateRemainingLazy(double now);
protected:
  double m_senderGap;
};

#endif /* SURF_NETWORK_CM02_HPP_ */
