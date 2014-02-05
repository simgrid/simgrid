/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "workstation_interface.hpp"
#include "storage_interface.hpp"
#include "cpu_interface.hpp"
#include "network_interface.hpp"

#ifndef SURF_WORKSTATION_CLM03_HPP_
#define SURF_WORKSTATION_CLM03_HPP_

/***********
 * Classes *
 ***********/

class WorkstationCLM03Model;
typedef WorkstationCLM03Model *WorkstationCLM03ModelPtr;

class WorkstationCLM03;
typedef WorkstationCLM03 *WorkstationCLM03Ptr;

class WorkstationCLM03Action;
typedef WorkstationCLM03Action *WorkstationCLM03ActionPtr;

/*********
 * Model *
 *********/

class WorkstationCLM03Model : public WorkstationModel {
public:
  WorkstationCLM03Model(const char *name);
  WorkstationCLM03Model();
  ~WorkstationCLM03Model();
  void parseInit(sg_platf_host_cbarg_t host);
  WorkstationPtr createResource(const char *name);
  double shareResources(double now);

  void updateActionsState(double now, double delta);

  ActionPtr executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *computation_amount,
                                        double *communication_amount,
                                        double rate);
 xbt_dynar_t getRoute(WorkstationPtr src, WorkstationPtr dst);
 ActionPtr communicate(WorkstationPtr src, WorkstationPtr dst, double size, double rate);
};

/************
 * Resource *
 ************/

class WorkstationCLM03 : public Workstation {
public:
  WorkstationCLM03(WorkstationModelPtr model, const char* name, xbt_dict_t properties, xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu);

  void updateState(tmgr_trace_event_t event_type, double value, double date);

  virtual ActionPtr execute(double size);
  virtual ActionPtr sleep(double duration);
  e_surf_resource_state_t getState();

  bool isUsed();

  xbt_dynar_t getVms();

  /* common with vm */
  void getParams(ws_params_t params);
  void setParams(ws_params_t params);
};


/**********
 * Action *
 **********/

class WorkstationCLM03Action : public WorkstationAction {
public:
  WorkstationCLM03Action(ModelPtr model, double cost, bool failed)
  : WorkstationAction(model, cost, failed) {}
};


#endif /* SURF_WORKSTATION_CLM03_HPP_ */
