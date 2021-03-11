/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/HostImpl.hpp"
#include <cstdlib>
#include <vector>
#include <xbt/base.h>

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
  HostL07Model(const HostL07Model&) = delete;
  HostL07Model& operator=(const HostL07Model&) = delete;

  double next_occurring_event(double now) override;
  void update_actions_state(double now, double delta) override;
  kernel::resource::CpuAction* execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                                                const double* bytes_amount, double rate) override;
};

class CpuL07Model : public kernel::resource::CpuModel {
public:
  CpuL07Model(HostL07Model* hmodel, kernel::lmm::System* sys);
  CpuL07Model(const CpuL07Model&) = delete;
  CpuL07Model& operator=(const CpuL07Model&) = delete;
  ~CpuL07Model() override;
  void update_actions_state(double /*now*/, double /*delta*/) override{
      /* this action is done by HostL07Model which shares the LMM system with the CPU model
       * Overriding to an empty function here allows us to handle the Cpu07Model as a regular
       * method in surf_presolve */
  };

  kernel::resource::Cpu* create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate) override;
  HostL07Model* hostModel_;
};

class NetworkL07Model : public kernel::resource::NetworkModel {
public:
  NetworkL07Model(HostL07Model* hmodel, kernel::lmm::System* sys);
  NetworkL07Model(const NetworkL07Model&) = delete;
  NetworkL07Model& operator=(const NetworkL07Model&) = delete;
  ~NetworkL07Model() override;
  kernel::resource::LinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths,
                                          s4u::Link::SharingPolicy policy) override;

  kernel::resource::Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) override;
  void update_actions_state(double /*now*/, double /*delta*/) override{
      /* this action is done by HostL07Model which shares the LMM system with the CPU model
       * Overriding to an empty function here allows us to handle the Cpu07Model as a regular
       * method in surf_presolve */
  };

  HostL07Model* hostModel_;
};

/************
 * Resource *
 ************/

class CpuL07 : public kernel::resource::Cpu {
public:
  using kernel::resource::Cpu::Cpu;
  CpuL07(const CpuL07&) = delete;
  CpuL07& operator=(const CpuL07&) = delete;

  bool is_used() const override;
  void apply_event(kernel::profile::Event* event, double value) override;
  kernel::resource::CpuAction* execution_start(double size) override;
  kernel::resource::CpuAction* execution_start(double, int) override
  {
    THROW_UNIMPLEMENTED;
    return nullptr;
  }
  kernel::resource::CpuAction* sleep(double duration) override;

protected:
  void on_speed_change() override;
};

class LinkL07 : public kernel::resource::LinkImpl {
public:
  LinkL07(const std::string& name, double bandwidth, s4u::Link::SharingPolicy policy, kernel::lmm::System* system);
  LinkL07(const LinkL07&) = delete;
  LinkL07& operator=(const LinkL07&) = delete;
  ~LinkL07() override;
  bool is_used() const override;
  void apply_event(kernel::profile::Event* event, double value) override;
  void set_bandwidth(double value) override;
  LinkImpl* set_latency(double value) override;
};

/**********
 * Action *
 **********/
class L07Action : public kernel::resource::CpuAction {
  std::vector<s4u::Host*> hostList_;
  bool free_arrays_ = false; // By default, computationAmount_ and friends are freed by caller. But not for sequential
                             // exec and regular comms
  const double* computationAmount_;   /* pointer to the data that lives in s4u action -- do not free unless if
                                       * free_arrays */
  const double* communicationAmount_; /* pointer to the data that lives in s4u action -- do not free unless if
                                       * free_arrays */
  double latency_;
  double rate_;

  friend CpuAction* CpuL07::execution_start(double size);
  friend CpuAction* CpuL07::sleep(double duration);
  friend CpuAction* HostL07Model::execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                                                   const double* bytes_amount, double rate);
  friend Action* NetworkL07Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate);

public:
  L07Action() = delete;
  L07Action(kernel::resource::Model* model, const std::vector<s4u::Host*>& host_list, const double* flops_amount,
            const double* bytes_amount, double rate);
  L07Action(const L07Action&) = delete;
  L07Action& operator=(const L07Action&) = delete;
  ~L07Action() override;

  void updateBound();
  double get_latency() const { return latency_; }
  void set_latency(double latency) { latency_ = latency; }
  void update_latency(double delta, double precision) { double_update(&latency_, delta, precision); }
};

} // namespace surf
} // namespace simgrid

#endif /* HOST_L07_HPP_ */
