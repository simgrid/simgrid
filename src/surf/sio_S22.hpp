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
  Action* execute_thread(const s4u::Host* host, double flops_amount, int thread_count) override;
  CpuAction* execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                              const double* bytes_amount, double rate) override { return nullptr; }
  DiskAction* io_stream(s4u::Host* src_host, DiskImpl* src_disk, s4u::Host* dst_host, DiskImpl* dst_disk,
                        double size) override;
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

  double latency_ = 0;

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
