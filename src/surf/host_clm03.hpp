/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "storage_interface.hpp"
#include "cpu_interface.hpp"
#include "host_interface.hpp"
#include "network_interface.hpp"

#ifndef SURF_HOST_CLM03_HPP_
#define SURF_HOST_CLM03_HPP_

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {

class XBT_PRIVATE HostCLM03Model;
class XBT_PRIVATE HostCLM03;
class XBT_PRIVATE HostCLM03Action;

/*********
 * Model *
 *********/

class HostCLM03Model : public HostModel {
public:
  HostCLM03Model(): HostModel(){}
  ~HostCLM03Model() {}
  Host *createHost(const char *name, RoutingEdge *netElm, Cpu *cpu, xbt_dict_t props) override;
  double shareResources(double now) override;

  void updateActionsState(double now, double delta) override;

  Action *executeParallelTask(int host_nb,
                              sg_host_t *host_list,
							  double *flops_amount,
							  double *bytes_amount,
							  double rate) override;
};

/************
 * Resource *
 ************/

class HostCLM03 : public Host {
public:
  HostCLM03(HostModel *model, const char* name, xbt_dict_t properties, xbt_dynar_t storage, RoutingEdge *netElm, Cpu *cpu);
  ~HostCLM03();
  void updateState(tmgr_trace_event_t event_type, double value, double date) override;

  virtual Action *execute(double size) override;
  virtual Action *sleep(double duration) override;

  bool isUsed() override;

  xbt_dynar_t getVms();
};


/**********
 * Action *
 **********/

}
}

#endif /* SURF_HOST_CLM03_HPP_ */
