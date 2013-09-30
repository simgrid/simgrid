#include "workstation.hpp"

#ifndef WORKSTATION_L07_HPP_
#define WORKSTATION_L07_HPP_

/***********
 * Classes *
 ***********/

class WorkstationL07Model;
typedef WorkstationL07Model *WorkstationL07ModelPtr;

class CpuL07;
typedef CpuL07 *CpuL07Ptr;

class LinkL07;
typedef LinkL07 *LinkL07Ptr;

class WorkstationL07ActionLmm;
typedef WorkstationL07ActionLmm *WorkstationL07ActionLmmPtr;

/*FIXME:class WorkstationActionLmm;
typedef WorkstationActionLmm *WorkstationActionLmmPtr;*/

/*********
 * Tools *
 *********/

/*********
 * Model *
 *********/
class WorkstationL07Model : public WorkstationModel {
public:
  WorkstationL07Model();
  void parseInit(sg_platf_host_cbarg_t host);
  WorkstationCLM03Ptr createCpuResource(const char *name, double power_scale,
                                 double power_initial,
                                 tmgr_trace_t power_trace,
                                 e_surf_resource_state_t state_initial,
                                 tmgr_trace_t state_trace,
                                 xbt_dict_t cpu_properties);
  WorkstationCLM03Ptr createLinkResource(const char *name,
		                                   double bw_initial,
		                                   tmgr_trace_t bw_trace,
		                                   double lat_initial,
		                                   tmgr_trace_t lat_trace,
		                                   e_surf_resource_state_t
		                                   state_initial,
		                                   tmgr_trace_t state_trace,
		                                   e_surf_link_sharing_policy_t
		                                   policy, xbt_dict_t properties);
  double shareResources(double now);
  void updateActionsState(double now, double delta);
  void addTraces();

  ActionPtr executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *computation_amount,
                                        double *communication_amount,
                                        double rate);
 xbt_dynar_t getRoute(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst);
 ActionPtr communicate(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst, double size, double rate);
};

class NetworkL07Model : public NetworkCm02Model {
public:
  NetworkL07Model(): NetworkCm02Model() {};
  xbt_dynar_t getRoute(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst) {DIE_IMPOSSIBLE;};
  ActionPtr communicate(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst, double size, double rate) {DIE_IMPOSSIBLE;};
  void addTraces() {DIE_IMPOSSIBLE;};
};

/************
 * Resource *
 ************/

class CpuL07 : public WorkstationCLM03Lmm {
public:
  CpuL07(WorkstationL07ModelPtr model, const char* name, xbt_dict_t properties);
  bool isUsed();
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  e_surf_resource_state_t getState();
  double getSpeed(double load);
  double getAvailableSpeed();
  ActionPtr execute(double size);
  ActionPtr sleep(double duration);

  double m_powerCurrent;
  RoutingEdgePtr p_info;
};

class LinkL07 : public WorkstationCLM03Lmm {
public:
  LinkL07(WorkstationL07ModelPtr model, const char* name, xbt_dict_t props);
  bool isUsed();
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  double getBandwidth();
  double getLatency();
  bool isShared();

  double m_latCurrent;
  tmgr_trace_event_t p_latEvent;
  double m_bwCurrent;
  tmgr_trace_event_t p_bwEvent;
};

/**********
 * Action *
 **********/
class WorkstationL07ActionLmm : public WorkstationActionLmm {
public:
  WorkstationL07ActionLmm(ModelPtr model, double cost, bool failed): WorkstationActionLmm(model, cost, failed) {};
 ~WorkstationL07ActionLmm();

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
  WorkstationCLM03Ptr *p_workstationList;
  double *p_computationAmount;
  double *p_communicationAmount;
  double m_latency;
  double m_rate;
};

#endif /* WORKSTATION_L07_HPP_ */
