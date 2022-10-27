/* Copyright (c) 2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "src/kernel/resource/NetworkModel.hpp"
#include "src/surf/HostImpl.hpp"
#include <xbt/base.h>

#ifndef SIO_S22_HPP_
#define SIO_S22_HPP_

namespace simgrid::kernel::resource {

/***********
 * Classes *
 ***********/

class XBT_PRIVATE HostS22Model;
class XBT_PRIVATE DiskS22Model;
class XBT_PRIVATE NetworkS22Model;

class XBT_PRIVATE DiskS22;
class XBT_PRIVATE LinkS22;

class XBT_PRIVATE S22Action;

/*********
 * Model *
 *********/
class HostS22Model : public HostModel {
public:
  HostS22Model(const std::string& name, lmm::System* sys);
  HostS22Model(const HostS22Model&) = delete;
  HostS22Model& operator=(const HostS22Model&) = delete;

  double next_occurring_event(double now) override;
  void update_actions_state(double now, double delta) override;
  Action* execute_thread(const s4u::Host* host, double flops_amount, int thread_count) override { return nullptr; }
  CpuAction* execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                              const double* bytes_amount, double rate) override { return nullptr; }
  S22Action* io_stream(s4u::Host* src_host, DiskImpl* src_disk, s4u::Host* dst_host, DiskImpl* dst_disk, double size);
};

class DiskS22Model : public DiskModel {
public:
  DiskS22Model(const std::string& name, HostS22Model* hmodel, lmm::System* sys);
  DiskS22Model(const DiskS22Model&) = delete;
  DiskS22Model& operator=(const DiskS22Model&) = delete;
  ~DiskS22Model() override;
  void update_actions_state(double /*now*/, double /*delta*/) override{
      /* this action is done by HostS22Model which shares the LMM system with the Disk model
       * Overriding to an empty function here allows us to handle the DiskS22Model as a regular
       * method in EngineImpl::presolve */
  };

  DiskImpl* create_disk(const std::string& name, double read_bandwidth, double write_bandwidth) override;
  HostS22Model* hostModel_;
};

class NetworkS22Model : public NetworkModel {
public:
  NetworkS22Model(const std::string& name, HostS22Model* hmodel, lmm::System* sys);
  NetworkS22Model(const NetworkS22Model&) = delete;
  NetworkS22Model& operator=(const NetworkS22Model&) = delete;
  ~NetworkS22Model() override;
  StandardLinkImpl* create_link(const std::string& name, const std::vector<double>& bandwidths) final;
  StandardLinkImpl* create_wifi_link(const std::string& name, const std::vector<double>& bandwidths) override;

  Action* communicate(s4u::Host* src, s4u::Host* dst, double size, double rate) override;
  void update_actions_state(double /*now*/, double /*delta*/) override{
      /* this action is done by HostS22Model which shares the LMM system with the Network model
       * Overriding to an empty function here allows us to handle the NetworkS22Model as a regular
       * method in EngineImpl::presolve */
  };

  HostS22Model* hostModel_;
};

/************
 * Resource *
 ************/

class DiskS22 : public DiskImpl {
public:
  using DiskImpl::DiskImpl;
  DiskS22(const DiskS22&) = delete;
  DiskS22& operator=(const DiskS22&) = delete;

  void apply_event(profile::Event* event, double value) override;
  DiskAction* io_start(sg_size_t size, s4u::Io::OpType type) override;
};

class LinkS22 : public StandardLinkImpl {
public:
  LinkS22(const std::string& name, double bandwidth, lmm::System* system);
  LinkS22(const LinkS22&) = delete;
  LinkS22& operator=(const LinkS22&) = delete;
  ~LinkS22() = default;

  void apply_event(profile::Event* event, double value) override;
  void set_bandwidth(double value) override;
  void set_latency(double value) override;
};

/**********
 * Action *
 **********/
class S22Action : public DiskAction {
  const s4u::Host* src_host_;
  const DiskImpl* src_disk_;
  const s4u::Host* dst_host_;
  const DiskImpl* dst_disk_;

  const double size_;

  double latency_;
  double rate_;

  friend DiskAction* DiskS22::io_start(sg_size_t size, s4u::Io::OpType type);
  friend Action* NetworkS22Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate);

  double calculate_io_read_bound() const;
  double calculate_io_write_bound() const;

  /**
   * @brief Calculate the network bound for the parallel task
   *
   * The network bound depends on the largest latency between the communication in the ptask.
   * Return MAX_DOUBLE if latency is 0 (or ptask doesn't have any communication)
   */
  double calculate_network_bound() const;

public:
  S22Action() = delete;
  S22Action(Model* model, s4u::Host* src_host, DiskImpl* src_disk, s4u::Host* dst_host, DiskImpl* dst_disk, double size);
  S22Action(const S22Action&) = delete;
  S22Action& operator=(const S22Action&) = delete;
  ~S22Action() = default;

  void update_bound() const;
  double get_latency() const { return latency_; }
  void set_latency(double latency) { latency_ = latency; }
  void update_latency(double delta, double precision) { double_update(&latency_, delta, precision); }
};

} // namespace simgrid::kernel::resource

#endif /* SIO_S22_HPP_ */
