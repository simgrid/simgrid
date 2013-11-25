#include "surf.hpp"
#include "storage.hpp"
#include "cpu.hpp"
#include "network.hpp"

#ifndef WORKSTATION_HPP_
#define WORKSTATION_HPP_

/***********
 * Classes *
 ***********/

class WorkstationModel;
typedef WorkstationModel *WorkstationModelPtr;

class WorkstationCLM03;
typedef WorkstationCLM03 *WorkstationCLM03Ptr;

class WorkstationCLM03Lmm;
typedef WorkstationCLM03Lmm *WorkstationCLM03LmmPtr;

class WorkstationAction;
typedef WorkstationAction *WorkstationActionPtr;

class WorkstationActionLmm;
typedef WorkstationActionLmm *WorkstationActionLmmPtr;

/*********
 * Tools *
 *********/
extern WorkstationModelPtr surf_workstation_model;

/*********
 * Model *
 *********/
class WorkstationModel : public Model {
public:
  WorkstationModel(string name): Model(name) {};
  WorkstationModel();
  ~WorkstationModel();
  virtual void parseInit(sg_platf_host_cbarg_t host);
  WorkstationCLM03Ptr createResource(string name);
  double shareResources(double now);
  virtual void adjustWeightOfDummyCpuActions();

  void updateActionsState(double now, double delta);

  virtual ActionPtr executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *computation_amount,
                                        double *communication_amount,
                                        double rate);
 virtual xbt_dynar_t getRoute(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst);
 virtual ActionPtr communicate(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst, double size, double rate);
 CpuModelPtr p_cpuModel;
};

/************
 * Resource *
 ************/

class WorkstationCLM03 : virtual public Resource {
public:
  WorkstationCLM03(WorkstationModelPtr model, const char* name, xbt_dict_t properties, xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu);

  void updateState(tmgr_trace_event_t event_type, double value, double date);

  virtual ActionPtr execute(double size);
  virtual ActionPtr sleep(double duration);
  e_surf_resource_state_t getState();

  virtual int getCore();
  virtual double getSpeed(double load);
  virtual double getAvailableSpeed();
  virtual double getCurrentPowerPeak();
  virtual double getPowerPeakAt(int pstate_index);
  virtual int getNbPstates();
  virtual void setPowerPeakAt(int pstate_index);
  virtual double getConsumedEnergy();

  xbt_dict_t getProperties();

  StoragePtr findStorageOnMountList(const char* storage);
  xbt_dict_t getStorageList();
  ActionPtr open(const char* mount, const char* path);
  ActionPtr close(surf_file_t fd);
  int unlink(surf_file_t fd);
  ActionPtr ls(const char* mount, const char *path);
  sg_size_t getSize(surf_file_t fd);
  ActionPtr read(surf_file_t fd, sg_size_t size);
  ActionPtr write(surf_file_t fd, sg_size_t size);
  xbt_dynar_t getInfo( surf_file_t fd);
  sg_size_t getFreeSize(const char* name);
  sg_size_t getUsedSize(const char* name);

  bool isUsed();
  //bool isShared();
  xbt_dynar_t p_storage;
  RoutingEdgePtr p_netElm;
  CpuPtr p_cpu;
  NetworkCm02LinkPtr p_network;

  xbt_dynar_t getVms();

  /* common with vm */
  void getParams(ws_params_t params);
  void setParams(ws_params_t params);
  s_ws_params_t p_params;
};

class WorkstationCLM03Lmm : virtual public WorkstationCLM03, public ResourceLmm {
public:
  WorkstationCLM03Lmm(WorkstationModelPtr model, const char* name, xbt_dict_t props, xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu):
	  WorkstationCLM03(model, name, props, storage, netElm, cpu){};
  e_surf_resource_state_t getState();
};

/**********
 * Action *
 **********/
class WorkstationAction : virtual public Action {
public:
  WorkstationAction(ModelPtr model, double cost, bool failed): Action(model, cost, failed) {};
};

class WorkstationActionLmm : public ActionLmm, public WorkstationAction {
public:
  WorkstationActionLmm(ModelPtr model, double cost, bool failed): ActionLmm(model, cost, failed), WorkstationAction(model, cost, failed) {};
};


#endif /* WORKSTATION_HPP_ */
