/* Copyright (c) 2013-2015. The SimGrid Team.
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

  double next_occuring_event(double now) override;
  void updateActionsState(double now, double delta) override;
  Action *executeParallelTask(int host_nb, sg_host_t *host_list,
                double *flops_amount, double *bytes_amount,
                double rate) override;
};

class CpuL07Model : public CpuModel {
public:
  CpuL07Model(HostL07Model *hmodel,lmm_system_t sys);
  ~CpuL07Model();

  Cpu *createCpu(simgrid::s4u::Host *host,  xbt_dynar_t speedPeakList,
                          tmgr_trace_t speedTrace, int core,
                          tmgr_trace_t state_trace) override;
  HostL07Model *p_hostModel;
};

class NetworkL07Model : public NetworkModel {
public:
  NetworkL07Model(HostL07Model *hmodel, lmm_system_t sys);
  ~NetworkL07Model();
  Link* createLink(const char *name, double bandwidth, double latency,
      e_surf_link_sharing_policy_t policy,
      xbt_dict_t properties) override;

  Action *communicate(NetCard *src, NetCard *dst, double size, double rate) override;
  bool next_occuring_event_isIdempotent() override {return true;}

  HostL07Model *p_hostModel;
};

/************
 * Resource *
 ************/

class CpuL07 : public Cpu {
public:
  CpuL07(CpuL07Model *model, simgrid::s4u::Host *host, xbt_dynar_t speedPeakList,
     tmgr_trace_t power_trace, int core, tmgr_trace_t state_trace);
  ~CpuL07();
  bool isUsed() override;
  void apply_event(tmgr_trace_iterator_t event, double value) override;
  Action *execution_start(double size) override;
  Action *sleep(double duration) override;
protected:
  void onSpeedChange() override;
};

class LinkL07 : public Link {
public:
  LinkL07(NetworkL07Model *model, const char* name, xbt_dict_t props,
      double bandwidth, double latency, e_surf_link_sharing_policy_t policy);
  ~LinkL07(){ };
  bool isUsed() override;
  void apply_event(tmgr_trace_iterator_t event, double value) override;
  void updateBandwidth(double value) override;
  void updateLatency(double value) override;
};

/**********
 * Action *
 **********/
class L07Action : public CpuAction {
  friend Action *CpuL07::execution_start(double size);
  friend Action *CpuL07::sleep(double duration);
  friend Action *HostL07Model::executeParallelTask(int host_nb,
                                                   sg_host_t*host_list,
                                                   double *flops_amount,
                                                   double *bytes_amount,
                                                   double rate);
public:
  L07Action(Model *model, int host_nb,
          sg_host_t*host_list,
          double *flops_amount,
       double *bytes_amount,
          double rate);
 ~L07Action();

  void updateBound();

  int unref() override;

  std::vector<NetCard*> * p_netcardList = new std::vector<NetCard*>();
  double *p_computationAmount;
  double *p_communicationAmount;
  double m_latency;
  double m_rate;
};

}
}

#endif /* HOST_L07_HPP_ */
