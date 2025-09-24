/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef HOST_L07_HPP_
#define HOST_L07_HPP_

#include "src/kernel/resource/HostImpl.hpp"
#include "src/kernel/resource/NetworkModel.hpp"
#include "src/simgrid/math_utils.h"
#include <cstdlib>
#include <vector>
#include <xbt/base.h>

namespace simgrid::kernel::resource {

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
 * Model *
 *********/
class HostL07Model : public HostModel {
public:
  HostL07Model(const std::string& name, lmm::System* sys);
  HostL07Model(const HostL07Model&)            = delete;
  HostL07Model& operator=(const HostL07Model&) = delete;

  double next_occurring_event(double now) override;
  void update_actions_state(double now, double delta) override;
  Action* execute_thread(const s4u::Host* host, double flops_amount, int thread_count) override { return nullptr; }
  CpuAction* execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                              const double* bytes_amount, double rate) override;
};

class CpuL07Model : public CpuModel {
public:
  CpuL07Model(const std::string& name, HostL07Model* hmodel, lmm::System* sys);
  CpuL07Model(const CpuL07Model&)            = delete;
  CpuL07Model& operator=(const CpuL07Model&) = delete;
  ~CpuL07Model() override;
  void update_actions_state(double /*now*/, double /*delta*/) override{
      /* this action is done by HostL07Model which shares the LMM system with the CPU model
       * Overriding to an empty function here allows us to handle the Cpu07Model as a regular
       * method in EngineImpl::presolve */
  };

  CpuImpl* create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate) override;
  HostL07Model* hostModel_;
};

class NetworkL07Model : public NetworkModel {
public:
  NetworkL07Model(const std::string& name, HostL07Model* hmodel, lmm::System* sys);
  NetworkL07Model(const NetworkL07Model&)            = delete;
  NetworkL07Model& operator=(const NetworkL07Model&) = delete;
  ~NetworkL07Model() override;
  StandardLinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths,
                                routing::NetZoneImpl* englobing_zone) final;
  StandardLinkImpl* create_wifi_link(const std::string& name, const std::vector<double>& bandwidths,
                                     routing::NetZoneImpl* englobing_zone) override;

  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool streamed) override;
  void update_actions_state(double /*now*/, double /*delta*/) override{
      /* this action is done by HostL07Model which shares the LMM system with the CPU model
       * Overriding to an empty function here allows us to handle the Cpu07Model as a regular
       * method in EngineImpl::presolve */
  };

  HostL07Model* hostModel_;
};

/************
 * Resource *
 ************/

class CpuL07 : public CpuImpl {
public:
  using CpuImpl::CpuImpl;
  CpuL07(const CpuL07&)            = delete;
  CpuL07& operator=(const CpuL07&) = delete;

  void apply_event(profile::Event* event, double value) override;
  CpuAction* execution_start(double size, double user_bound) override;
  CpuAction* execution_start(double, int, double) override
  {
    THROW_UNIMPLEMENTED;
    return nullptr;
  }
  CpuAction* sleep(double duration) override;

protected:
  void on_speed_change() override;
};

class LinkL07 : public StandardLinkImpl {
public:
  LinkL07(const std::string& name, double bandwidth, lmm::System* system, routing::NetZoneImpl* englobing_zone);
  LinkL07(const LinkL07&)            = delete;
  LinkL07& operator=(const LinkL07&) = delete;
  ~LinkL07() override;
  void apply_event(profile::Event* event, double value) override;
  void set_bandwidth(double value) override;
  void set_latency(double value) override;
};

/**********
 * Action *
 **********/
class L07Action : public CpuAction {
  const std::vector<s4u::Host*> host_list_;
  bool free_arrays_ = false; // By default, computation_amount_ and friends are freed by caller. But not for sequential
                             // exec and regular comms
  const double* computation_amount_;   /* pointer to the data that lives in s4u action -- do not free unless if
                                        * free_arrays */
  const double* communication_amount_; /* pointer to the data that lives in s4u action -- do not free unless if
                                        * free_arrays */
  double latency_;
  double rate_;

  friend CpuAction* CpuL07::execution_start(double size, double user_bound);
  friend CpuAction* CpuL07::sleep(double duration);
  friend CpuAction* HostL07Model::execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                                                   const double* bytes_amount, double rate);
  friend Action* NetworkL07Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool streamed);
  /**
   * @brief Calculate the CPU bound for the parallel task
   *
   * The task is bounded by the slowest CPU running the ptask, considering the current pstate of each CPU.
   * Return MAX_DOUBLE if ptask has no computation.
   */
  double calculate_cpu_bound() const;

  /**
   * @brief Calculate the network bound for the parallel task
   *
   * The network bound depends on the largest latency between the communication in the ptask.
   * Return MAX_DOUBLE if latency is 0 (or ptask doesn't have any communication)
   */
  double calculate_network_bound() const;

public:
  L07Action() = delete;
  L07Action(Model* model, const std::vector<s4u::Host*>& host_list, const double* flops_amount,
            const double* bytes_amount, double rate);
  L07Action(const L07Action&)            = delete;
  L07Action& operator=(const L07Action&) = delete;
  ~L07Action() override;

  void update_bound() const;
  double get_latency() const { return latency_; }
  void set_latency(double latency) { latency_ = latency; }
  void update_latency(double delta, double precision) { double_update(&latency_, delta, precision); }
};

} // namespace simgrid::kernel::resource

#endif /* HOST_L07_HPP_ */
