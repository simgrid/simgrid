/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>
#include <vector>
#include <xbt/base.h>
#include "src/surf/HostImpl.hpp"

#ifndef HOST_L07_HPP_
#define HOST_L07_HPP_

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/

class XBT_PRIVATE HostL07Model;
class XBT_PRIVATE CpuL07Model;
class XBT_PRIVATE NetworkL07Model;

class XBT_PRIVATE CpuL07;
class XBT_PRIVATE LinkL07;

class XBT_PRIVATE L07Action;
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

  double nextOccuringEvent(double now) override;
  void updateActionsState(double now, double delta) override;
  Action *executeParallelTask(int host_nb, sg_host_t *host_list,
                              double *flops_amount, double *bytes_amount, double rate) override;
};

class CpuL07Model : public CpuModel {
public:
  CpuL07Model(HostL07Model* hmodel, lmm_system_t sys);
  ~CpuL07Model();

  Cpu *createCpu(simgrid::s4u::Host *host, std::vector<double> *speedPerPstate, int core) override;
  HostL07Model *hostModel_;
};

class NetworkL07Model : public NetworkModel {
public:
  NetworkL07Model(HostL07Model* hmodel, lmm_system_t sys);
  ~NetworkL07Model();
  LinkImpl* createLink(const std::string& name, double bandwidth, double latency,
                       e_surf_link_sharing_policy_t policy) override;

  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) override;

  HostL07Model *hostModel_;
};

/************
 * Resource *
 ************/

class CpuL07 : public Cpu {
public:
  CpuL07(CpuL07Model *model, simgrid::s4u::Host *host, std::vector<double> * speedPerPstate, int core);
  ~CpuL07() override;
  bool isUsed() override;
  void apply_event(tmgr_trace_event_t event, double value) override;
  Action *execution_start(double size) override;
  Action *sleep(double duration) override;
protected:
  void onSpeedChange() override;
};

class LinkL07 : public LinkImpl {
public:
  LinkL07(NetworkL07Model* model, const std::string& name, double bandwidth, double latency,
          e_surf_link_sharing_policy_t policy);
  ~LinkL07() override;
  bool isUsed() override;
  void apply_event(tmgr_trace_event_t event, double value) override;
  void setBandwidth(double value) override;
  void setLatency(double value) override;
};

/**********
 * Action *
 **********/
class L07Action : public CpuAction {
  friend Action *CpuL07::execution_start(double size);
  friend Action *CpuL07::sleep(double duration);
  friend Action *HostL07Model::executeParallelTask(int host_nb, sg_host_t*host_list,
                                                   double *flops_amount, double *bytes_amount, double rate);
public:
  L07Action(Model *model, int host_nb, sg_host_t *host_list, double *flops_amount, double *bytes_amount, double rate);
 ~L07Action();

  void updateBound();

  int unref() override;

  std::vector<s4u::Host*>* hostList_ = new std::vector<s4u::Host*>();
  double *computationAmount_;
  double *communicationAmount_;
  double latency_;
  double rate_;
};

}
}

#endif /* HOST_L07_HPP_ */
