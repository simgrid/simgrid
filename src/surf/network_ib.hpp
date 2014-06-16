/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_IB_HPP_
#define SURF_NETWORK_IB_HPP_

class NetworkIBModel : public NetworkModel {
private:
public:
  NetworkIBModel();
  NetworkIBModel(const char *name);
  ~NetworkModel();
  virtual ActionPtr communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
                                double size, double rate);
  virtual NetworkLinkPtr createNetworkLink(const char *name,
                                          double bw_initial,
                                          tmgr_trace_t bw_trace,
                                          double lat_initial,
                                          tmgr_trace_t lat_trace,
                                          e_surf_resource_state_t state_initial,
                                          tmgr_trace_t state_trace,
                                          e_surf_link_sharing_policy_t policy,
                                          xbt_dict_t properties);
};

class NetworkIBLink : public NetworkLink {
private:
public:
  NetworkIBLink(NetworkModelPtr model, const char *name, xbt_dict_t props);
  NetworkIBLink(NetworkModelPtr model, const char *name, xbt_dict_t props,
                lmm_constraint_t constraint,
                tmgr_history_t history,
                tmgr_trace_t state_trace);
  ~NetworkIBLink();
  virtual void updateLatency(double value, double date=surf_get_clock());
  virtual void updateBandwidth(double value, double date=surf_get_clock());


};
#endif
