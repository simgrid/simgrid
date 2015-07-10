/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "host_interface.hpp"

#ifndef HOST_L07_HPP_
#define HOST_L07_HPP_

/***********
 * Classes *
 ***********/

class HostL07Model;
typedef HostL07Model *HostL07ModelPtr;

class CpuL07Model;
typedef CpuL07Model *CpuL07ModelPtr;

class NetworkL07Model;
typedef NetworkL07Model *NetworkL07ModelPtr;

class HostL07;
typedef HostL07 *HostL07Ptr;

class CpuL07;
typedef CpuL07 *CpuL07Ptr;

class LinkL07;
typedef LinkL07 *LinkL07Ptr;

class HostL07Action;
typedef HostL07Action *HostL07ActionPtr;

/*********
 * Tools *
 *********/

/*********
 * Model *
 *********/
class HostL07Model : public HostModel {
public:
  HostL07Model();
  ~HostL07Model();

  double shareResources(double now);
  void updateActionsState(double now, double delta);
  HostPtr createHost(const char *name);
  ActionPtr executeParallelTask(int host_nb,
                                        void **host_list,
                                        double *flops_amount,
                                        double *bytes_amount,
                                        double rate);
  xbt_dynar_t getRoute(HostPtr src, HostPtr dst);
  ActionPtr communicate(HostPtr src, HostPtr dst, double size, double rate);
  void addTraces();
  NetworkModelPtr p_networkModel;
};

class CpuL07Model : public CpuModel {
public:
  CpuL07Model() : CpuModel("cpuL07") {};
  ~CpuL07Model() {surf_cpu_model_pm = NULL;};
  CpuPtr createCpu(const char *name,  xbt_dynar_t powerPeak,
                          int pstate, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties);
  void addTraces() {DIE_IMPOSSIBLE;};

  HostL07ModelPtr p_hostModel;
};

class NetworkL07Model : public NetworkModel {
public:
  NetworkL07Model() : NetworkModel() {};
  ~NetworkL07Model() {surf_network_model = NULL;};
  NetworkLinkPtr createNetworkLink(const char *name,
		                                   double bw_initial,
		                                   tmgr_trace_t bw_trace,
		                                   double lat_initial,
		                                   tmgr_trace_t lat_trace,
		                                   e_surf_resource_state_t
		                                   state_initial,
		                                   tmgr_trace_t state_trace,
		                                   e_surf_link_sharing_policy_t
		                                   policy, xbt_dict_t properties);

  ActionPtr communicate(RoutingEdgePtr /*src*/, RoutingEdgePtr /*dst*/, double /*size*/, double /*rate*/) {DIE_IMPOSSIBLE;};
  void addTraces() {DIE_IMPOSSIBLE;};
  HostL07ModelPtr p_hostModel;
};

/************
 * Resource *
 ************/

class HostL07 : public Host {
public:
  HostL07(HostModelPtr model, const char* name, xbt_dict_t props, RoutingEdgePtr netElm, CpuPtr cpu);
  //bool isUsed();
  bool isUsed() {DIE_IMPOSSIBLE;};
  void updateState(tmgr_trace_event_t /*event_type*/, double /*value*/, double /*date*/) {DIE_IMPOSSIBLE;};
  ActionPtr execute(double size);
  ActionPtr sleep(double duration);
  e_surf_resource_state_t getState();
  double getPowerPeakAt(int pstate_index);
  int getNbPstates();
  void setPstate(int pstate_index);
  int  getPstate();
  double getConsumedEnergy();
};

class CpuL07 : public Cpu {
  friend void HostL07Model::addTraces();
  tmgr_trace_event_t p_stateEvent;
  tmgr_trace_event_t p_powerEvent;
public:
  CpuL07(CpuL07ModelPtr model, const char* name, xbt_dict_t properties,
		 double power_scale, double power_initial, tmgr_trace_t power_trace,
     int core, e_surf_resource_state_t state_initial, tmgr_trace_t state_trace);
  bool isUsed();
  //bool isUsed() {DIE_IMPOSSIBLE;};
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  CpuActionPtr execute(double /*size*/) {DIE_IMPOSSIBLE;};
  CpuActionPtr sleep(double /*duration*/) {DIE_IMPOSSIBLE;};

  double getCurrentPowerPeak() {THROW_UNIMPLEMENTED;};
  double getPowerPeakAt(int /*pstate_index*/) {THROW_UNIMPLEMENTED;};
  int getNbPstates() {THROW_UNIMPLEMENTED;};
  void setPstate(int /*pstate_index*/) {THROW_UNIMPLEMENTED;};
  int  getPstate() {THROW_UNIMPLEMENTED;};
  double getConsumedEnergy() {THROW_UNIMPLEMENTED;};
};

class LinkL07 : public NetworkLink {
public:
  LinkL07(NetworkL07ModelPtr model, const char* name, xbt_dict_t props,
		  double bw_initial,
          tmgr_trace_t bw_trace,
          double lat_initial,
          tmgr_trace_t lat_trace,
          e_surf_resource_state_t
          state_initial,
          tmgr_trace_t state_trace,
          e_surf_link_sharing_policy_t policy);
  ~LinkL07(){
  };
  bool isUsed();
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  double getBandwidth();
  double getLatency();
  bool isShared();
  void updateBandwidth(double value, double date=surf_get_clock());
  void updateLatency(double value, double date=surf_get_clock());

  double m_latCurrent;
  tmgr_trace_event_t p_latEvent;
  double m_bwCurrent;
  tmgr_trace_event_t p_bwEvent;
};

/**********
 * Action *
 **********/
class HostL07Action : public HostAction {
  friend ActionPtr HostL07::execute(double size);
  friend ActionPtr HostL07::sleep(double duration);
  friend ActionPtr HostL07Model::executeParallelTask(int host_nb,
                                                     void **host_list,
                                                   double *flops_amount,
												   double *bytes_amount,
                                                   double rate);
public:
  HostL07Action(ModelPtr model, double cost, bool failed)
  : HostAction(model, cost, failed) {};
 ~HostL07Action();

  void updateBound();

  int unref();
  void cancel();
  void suspend();
  void resume();
  bool isSuspended();
  void setMaxDuration(double duration);
  void setPriority(double priority);
  double getRemains();

  int m_hostNb;
  HostPtr *p_hostList;
  double *p_computationAmount;
  double *p_communicationAmount;
  double m_latency;
  double m_rate;
};

#endif /* HOST_L07_HPP_ */
