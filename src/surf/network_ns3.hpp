/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "network_interface.hpp"
#include "surf/ns3/ns3_interface.h"

#ifndef NETWORK_NS3_HPP_
#define NETWORK_NS3_HPP_

/***********
 * Classes *
 ***********/
class XBT_PRIVATE NetworkNS3Model;
class XBT_PRIVATE NetworkNS3Action;

/*********
 * Tools *
 *********/

XBT_PRIVATE void net_define_callbacks(void);

/*********
 * Model *
 *********/

class NetworkNS3Model : public NetworkModel {
public:
  NetworkNS3Model();

  ~NetworkNS3Model();
  Link* createLink(const char *name,
  	                                 double bw_initial,
  	                                 tmgr_trace_t bw_trace,
  	                                 double lat_initial,
  	                                 tmgr_trace_t lat_trace,
  	                                 e_surf_resource_state_t state_initial,
  	                                 tmgr_trace_t state_trace,
  	                                 e_surf_link_sharing_policy_t policy,
  	                                 xbt_dict_t properties);
  xbt_dynar_t getRoute(RoutingEdge *src, RoutingEdge *dst);
  Action *communicate(RoutingEdge *src, RoutingEdge *dst,
		                           double size, double rate);
  double shareResources(double now);
  void updateActionsState(double now, double delta);
  void addTraces(){DIE_IMPOSSIBLE;}
  bool shareResourcesIsIdempotent() {return false;}
};

/************
 * Resource *
 ************/
class NetworkNS3Link : public Link {
public:
  NetworkNS3Link(NetworkNS3Model *model, const char *name, xbt_dict_t props,
  		         double bw_initial, double lat_initial);
  ~NetworkNS3Link();

  void updateState(tmgr_trace_event_t event_type, double value, double date);
  double getLatency(){THROW_UNIMPLEMENTED;}
  double getBandwidth(){THROW_UNIMPLEMENTED;}
  void updateBandwidth(double value, double date=surf_get_clock()){THROW_UNIMPLEMENTED;}
  void updateLatency(double value, double date=surf_get_clock()){THROW_UNIMPLEMENTED;}

//private:
 char *p_id;
 char *p_lat;
 char *p_bdw;
 int m_created;
};

/**********
 * Action *
 **********/
class NetworkNS3Action : public NetworkAction {
public:
  NetworkNS3Action(Model *model, double cost, bool failed);

#ifdef HAVE_LATENCY_BOUND_TRACKING
  int getLatencyLimited();
#endif

bool isSuspended();
int unref();
void suspend();
void resume();

//private:
  double m_lastSent;
  RoutingEdge *p_srcElm;
  RoutingEdge *p_dstElm;
};


#endif /* NETWORK_NS3_HPP_ */
