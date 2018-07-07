/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

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
  void update_actions_state(double now, double delta) override;
  kernel::resource::Action* execute_parallel(int host_nb, sg_host_t* host_list, double* flops_amount,
                                             double* bytes_amount, double rate) override;
};

class CpuL07Model : public CpuModel {
public:
  CpuL07Model(HostL07Model* hmodel, kernel::lmm::System* sys);
  ~CpuL07Model();

  Cpu* create_cpu(simgrid::s4u::Host* host, std::vector<double>* speed_per_pstate, int core) override;
  HostL07Model *hostModel_;
};

class NetworkL07Model : public kernel::resource::NetworkModel {
public:
  NetworkL07Model(HostL07Model* hmodel, kernel::lmm::System* sys);
  ~NetworkL07Model();
  kernel::resource::LinkImpl* create_link(const std::string& name, double bandwidth, double latency,
                                          s4u::Link::SharingPolicy policy) override;

  kernel::resource::Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) override;

  HostL07Model *hostModel_;
};

/************
 * Resource *
 ************/

class CpuL07 : public Cpu {
public:
  CpuL07(CpuL07Model* model, simgrid::s4u::Host* host, std::vector<double>* speed_per_pstate, int core);
  ~CpuL07() override;
  bool is_used() override;
  void apply_event(tmgr_trace_event_t event, double value) override;
  kernel::resource::Action* execution_start(double size) override;
  simgrid::kernel::resource::Action* execution_start(double size, int requested_cores) override
  {
    THROW_UNIMPLEMENTED;
    return nullptr;
  }
  kernel::resource::Action* sleep(double duration) override;

protected:
  void on_speed_change() override;
};

class LinkL07 : public kernel::resource::LinkImpl {
public:
  LinkL07(NetworkL07Model* model, const std::string& name, double bandwidth, double latency,
          s4u::Link::SharingPolicy policy);
  ~LinkL07() override;
  bool is_used() override;
  void apply_event(tmgr_trace_event_t event, double value) override;
  void set_bandwidth(double value) override;
  void set_latency(double value) override;
};

/**********
 * Action *
 **********/
class L07Action : public CpuAction {
  friend Action *CpuL07::execution_start(double size);
  friend Action *CpuL07::sleep(double duration);
  friend Action* HostL07Model::execute_parallel(int host_nb, sg_host_t* host_list, double* flops_amount,
                                                double* bytes_amount, double rate);

public:
  L07Action(kernel::resource::Model* model, int host_nb, sg_host_t* host_list, double* flops_amount,
            double* bytes_amount, double rate);
  ~L07Action();

  void updateBound();

  std::vector<s4u::Host*>* hostList_ = new std::vector<s4u::Host*>();
  double *computationAmount_;
  double *communicationAmount_;
  double latency_;
  double rate_;
};

}
}

#endif /* HOST_L07_HPP_ */
