#include "workstation.hpp"

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

  double shareResources(double now);
  void updateActionsState(double now, double delta);
  ResourcePtr createResource(const char *name, double power_scale,
                                 double power_initial,
                                 tmgr_trace_t power_trace,
                                 e_surf_resource_state_t state_initial,
                                 tmgr_trace_t state_trace,
                                 xbt_dict_t cpu_properties);
  ActionPtr executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *computation_amount,
                                        double *communication_amount,
                                        double rate);
  xbt_dynar_t getRoute(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst);
  ActionPtr communicate(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst, double size, double rate);
  void addTraces();
  CpuL07ModelPtr p_cpuModel;
  NetworkL07ModelPtr p_networkModel;
};

class CpuL07Model : public CpuModel {
public:
  CpuL07Model() : CpuModel("cpuL07") {};
  ResourcePtr createResource(const char *name, double power_scale,
                                 double power_initial,
                                 tmgr_trace_t power_trace,
                                 e_surf_resource_state_t state_initial,
                                 tmgr_trace_t state_trace,
                                 xbt_dict_t cpu_properties);
  void addTraces() {DIE_IMPOSSIBLE;};
  WorkstationL07ModelPtr p_workstationModel;
};

class NetworkL07Model : public NetworkCm02Model {
public:
  NetworkL07Model() : NetworkCm02Model(0) {};
  ResourcePtr createResource(const char *name,
		                                   double bw_initial,
		                                   tmgr_trace_t bw_trace,
		                                   double lat_initial,
		                                   tmgr_trace_t lat_trace,
		                                   e_surf_resource_state_t
		                                   state_initial,
		                                   tmgr_trace_t state_trace,
		                                   e_surf_link_sharing_policy_t
		                                   policy, xbt_dict_t properties);
  NetworkCm02ActionPtr communicate(RoutingEdgePtr src, RoutingEdgePtr dst,
		                           double size, double rate);
  xbt_dynar_t getRoute(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst) {DIE_IMPOSSIBLE;};
  ActionPtr communicate(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst, double size, double rate) {DIE_IMPOSSIBLE;};
  void addTraces() {DIE_IMPOSSIBLE;};
  WorkstationL07ModelPtr p_workstationModel;
};

/************
 * Resource *
 ************/

class WorkstationL07 : public WorkstationCLM03Lmm {
public:
  WorkstationL07(WorkstationModelPtr model, const char* name, xbt_dict_t props, RoutingEdgePtr netElm, CpuPtr cpu);
  bool isUsed();
  void updateState(tmgr_trace_event_t event_type, double value, double date) {DIE_IMPOSSIBLE;};
  ActionPtr execute(double size);
  ActionPtr sleep(double duration);
  e_surf_resource_state_t getState();
};

class CpuL07 : public CpuLmm {
public:
  CpuL07(CpuL07ModelPtr model, const char* name, xbt_dict_t properties);
  bool isUsed() {DIE_IMPOSSIBLE;};
  void updateState(tmgr_trace_event_t event_type, double value, double date);
  e_surf_resource_state_t getState();
  double getSpeed(double load);
  double getAvailableSpeed();
  ActionPtr execute(double size) {DIE_IMPOSSIBLE;};
  ActionPtr sleep(double duration) {DIE_IMPOSSIBLE;};
  double m_powerCurrent;
};

class LinkL07 : public NetworkCm02LinkLmm {
public:
  LinkL07(NetworkL07ModelPtr model, const char* name, xbt_dict_t props);
  bool isUsed() {DIE_IMPOSSIBLE;};
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
  WorkstationL07ActionLmm(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed), WorkstationActionLmm(model, cost, failed) {};
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
