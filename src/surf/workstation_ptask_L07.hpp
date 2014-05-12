/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "workstation_interface.hpp"

#ifndef WORKSTATION_L07_HPP_
#define WORKSTATION_L07_HPP_

/***********
 * Classes *
 ***********/

class WorkstationL07Model;
typedef WorkstationL07Model *WorkstationL07ModelPtr;

class CpuL07Model;
typedef CpuL07Model *CpuL07ModelPtr;

class NetworkL07Model;
typedef NetworkL07Model *NetworkL07ModelPtr;

class WorkstationL07;
typedef WorkstationL07 *WorkstationL07Ptr;

class CpuL07;
typedef CpuL07 *CpuL07Ptr;

class LinkL07;
typedef LinkL07 *LinkL07Ptr;

class WorkstationL07Action;
typedef WorkstationL07Action *WorkstationL07ActionPtr;

/*********
 * Tools *
 *********/

/*********
 * Model *
 *********/
class WorkstationL07Model : public WorkstationModel {
public:
  WorkstationL07Model();
  ~WorkstationL07Model();

  double shareResources(double now);
  void updateActionsState(double now, double delta);
  WorkstationPtr createWorkstation(const char *name);
  ActionPtr executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *computation_amount,
                                        double *communication_amount,
                                        double rate);
  xbt_dynar_t getRoute(WorkstationPtr src, WorkstationPtr dst);
  ActionPtr communicate(WorkstationPtr src, WorkstationPtr dst, double size, double rate);
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

  WorkstationL07ModelPtr p_workstationModel;
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
  WorkstationL07ModelPtr p_workstationModel;
};

/************
 * Resource *
 ************/

class WorkstationL07 : public Workstation {
public:
  WorkstationL07(WorkstationModelPtr model, const char* name, xbt_dict_t props, RoutingEdgePtr netElm, CpuPtr cpu);
  //bool isUsed();
  bool isUsed() {DIE_IMPOSSIBLE;};
  void updateState(tmgr_trace_event_t /*event_type*/, double /*value*/, double /*date*/) {DIE_IMPOSSIBLE;};
  ActionPtr execute(double size);
  ActionPtr sleep(double duration);
  e_surf_resource_state_t getState();
  double getPowerPeakAt(int pstate_index);
  int getNbPstates();
  void setPowerPeakAt(int pstate_index);
  double getConsumedEnergy();
};

class CpuL07 : public Cpu {
  friend void WorkstationL07Model::addTraces();
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
  void setPowerPeakAt(int /*pstate_index*/) {THROW_UNIMPLEMENTED;};
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
class WorkstationL07Action : public WorkstationAction {
  friend ActionPtr WorkstationL07::execute(double size);
  friend ActionPtr WorkstationL07::sleep(double duration);
  friend ActionPtr WorkstationL07Model::executeParallelTask(int workstation_nb,
                                                     void **workstation_list,
                                                   double
                                                   *computation_amount, double
                                                   *communication_amount,
                                                   double rate);
public:
  WorkstationL07Action(ModelPtr model, double cost, bool failed)
  : WorkstationAction(model, cost, failed) {};
 ~WorkstationL07Action();

  void updateBound();

  int unref();
  void cancel();
  void suspend();
  void resume();
  bool isSuspended();
  void setMaxDuration(double duration);
  void setPriority(double priority);
  double getRemains();

  int m_workstationNb;
  WorkstationPtr *p_workstationList;
  double *p_computationAmount;
  double *p_communicationAmount;
  double m_latency;
  double m_rate;
};

#endif /* WORKSTATION_L07_HPP_ */
