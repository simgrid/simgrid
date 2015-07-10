/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "storage_interface.hpp"
#include "cpu_interface.hpp"
#include "host_interface.hpp"
#include "network_interface.hpp"

#ifndef SURF_HOST_CLM03_HPP_
#define SURF_HOST_CLM03_HPP_

/***********
 * Classes *
 ***********/

class HostCLM03Model;
typedef HostCLM03Model *HostCLM03ModelPtr;

class HostCLM03;
typedef HostCLM03 *HostCLM03Ptr;

class HostCLM03Action;
typedef HostCLM03Action *HostCLM03ActionPtr;

/*********
 * Model *
 *********/

class HostCLM03Model : public HostModel {
public:
  HostCLM03Model();
  ~HostCLM03Model();
  HostPtr createHost(const char *name);
  double shareResources(double now);

  void updateActionsState(double now, double delta);

  ActionPtr executeParallelTask(int host_nb,
                                        void **host_list,
                                        double *flops_amount,
                                        double *bytes_amount,
                                        double rate);
 ActionPtr communicate(HostPtr src, HostPtr dst, double size, double rate);
};

/************
 * Resource *
 ************/

class HostCLM03 : public Host {
public:
  HostCLM03(HostModelPtr model, const char* name, xbt_dict_t properties, xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu);

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



#endif /* SURF_HOST_CLM03_HPP_ */
