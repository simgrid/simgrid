#include "surf_interface.hpp"
#include "storage_interface.hpp"
#include "cpu_interface.hpp"
#include "network_interface.hpp"

#ifndef SURF_WORKSTATION_INTERFACE_HPP_
#define SURF_WORKSTATION_INTERFACE_HPP_

/***********
 * Classes *
 ***********/

class WorkstationModel;
typedef WorkstationModel *WorkstationModelPtr;

class Workstation;
typedef Workstation *WorkstationPtr;

class WorkstationAction;
typedef WorkstationAction *WorkstationActionPtr;

/*********
 * Tools *
 *********/
extern WorkstationModelPtr surf_workstation_model;

/*********
 * Model *
 *********/
class WorkstationModel : public Model {
public:
  WorkstationModel(const char *name);
  WorkstationModel();
  ~WorkstationModel();
  virtual void adjustWeightOfDummyCpuActions();

  virtual ActionPtr executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *computation_amount,
                                        double *communication_amount,
                                        double rate)=0;
 virtual xbt_dynar_t getRoute(WorkstationPtr src, WorkstationPtr dst)=0;
 virtual ActionPtr communicate(WorkstationPtr src, WorkstationPtr dst, double size, double rate)=0;

 CpuModelPtr p_cpuModel;
};

/************
 * Resource *
 ************/

class Workstation : public Resource {
public:
  Workstation(){};
  Workstation(ModelPtr model, const char *name, xbt_dict_t props,
		      xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu);
  Workstation(ModelPtr model, const char *name, xbt_dict_t props, lmm_constraint_t constraint,
		      xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu);

  xbt_dict_t getProperties();

  virtual ActionPtr execute(double size)=0;
  virtual ActionPtr sleep(double duration)=0;

  virtual int getCore();
  virtual double getSpeed(double load);
  virtual double getAvailableSpeed();
  virtual double getCurrentPowerPeak();
  virtual double getPowerPeakAt(int pstate_index);
  virtual int getNbPstates();
  virtual void setPowerPeakAt(int pstate_index);
  virtual double getConsumedEnergy();

  virtual StoragePtr findStorageOnMountList(const char* storage);
  virtual xbt_dict_t getStorageList();
  virtual ActionPtr open(const char* mount, const char* path);
  virtual ActionPtr close(surf_file_t fd);
  virtual int unlink(surf_file_t fd);
  virtual ActionPtr ls(const char* mount, const char *path);
  virtual sg_size_t getSize(surf_file_t fd);
  virtual ActionPtr read(surf_file_t fd, sg_size_t size);
  virtual ActionPtr write(surf_file_t fd, sg_size_t size);
  virtual xbt_dynar_t getInfo( surf_file_t fd);
  virtual sg_size_t fileTell(surf_file_t fd);
  virtual sg_size_t getFreeSize(const char* name);
  virtual sg_size_t getUsedSize(const char* name);
  virtual int fileSeek(surf_file_t fd, sg_size_t offset, int origin);

  xbt_dynar_t p_storage;
  RoutingEdgePtr p_netElm;
  CpuPtr p_cpu;
  NetworkLinkPtr p_network;

  xbt_dynar_t getVms();

  /* common with vm */
  void getParams(ws_params_t params);
  void setParams(ws_params_t params);
  s_ws_params_t p_params;
};

/**********
 * Action *
 **********/
class WorkstationAction : public Action {
public:
  WorkstationAction(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed) {}
  WorkstationAction(ModelPtr model, double cost, bool failed, lmm_variable_t var)
  : Action(model, cost, failed, var) {}

};


#endif /* SURF_WORKSTATION_INTERFACE_HPP_ */
