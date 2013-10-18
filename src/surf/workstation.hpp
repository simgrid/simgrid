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

/*FIXME:class WorkstationActionLmm;
typedef WorkstationActionLmm *WorkstationActionLmmPtr;*/

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
  void updateActionsState(double now, double delta);

  virtual ActionPtr executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *computation_amount,
                                        double *communication_amount,
                                        double rate);
 virtual xbt_dynar_t getRoute(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst);
 virtual ActionPtr communicate(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst, double size, double rate);
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

  xbt_dict_t getProperties();

  StoragePtr findStorageOnMountList(const char* storage);
  ActionPtr open(const char* mount, const char* path);
  ActionPtr close(surf_file_t fd);
  int unlink(surf_file_t fd);
  ActionPtr ls(const char* mount, const char *path);
  size_t getSize(surf_file_t fd);
  ActionPtr read(void* ptr, size_t size, surf_file_t fd);
  ActionPtr write(const void* ptr, size_t size, surf_file_t fd);
  bool isUsed();
  //bool isShared();
  xbt_dynar_t p_storage;
  RoutingEdgePtr p_netElm;
  CpuPtr p_cpu;
  NetworkCm02LinkPtr p_network;
};

class WorkstationCLM03Lmm : public WorkstationCLM03, public ResourceLmm {
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
