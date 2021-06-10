/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/kernel/resource/Model.hpp"
#include "simgrid/kernel/resource/Resource.hpp"
#include "simgrid/s4u/Disk.hpp"
#include "simgrid/s4u/Io.hpp"
#include "src/surf/surf_interface.hpp"
#include <xbt/PropertyHolder.hpp>

#include <map>

#ifndef DISK_INTERFACE_HPP_
#define DISK_INTERFACE_HPP_

/*********
 * Model *
 *********/

namespace simgrid {
namespace kernel {
namespace resource {
/***********
 * Classes *
 ***********/

class DiskAction;

/*********
 * Model *
 *********/
class DiskModel : public Model {
public:
  explicit DiskModel(const std::string& name);
  DiskModel(const DiskModel&) = delete;
  DiskModel& operator=(const DiskModel&) = delete;

  virtual DiskImpl* create_disk(const std::string& name, double read_bandwidth, double write_bandwidth) = 0;

  virtual DiskAction* io_start(const DiskImpl* disk, sg_size_t size, s4u::Io::OpType type) = 0;
};

/************
 * Resource *
 ************/
class DiskImpl : public Resource_T<DiskImpl>, public xbt::PropertyHolder {
  s4u::Disk piface_;
  s4u::Host* host_                   = nullptr;
  lmm::Constraint* constraint_write_ = nullptr; /* Constraint for maximum write bandwidth*/
  lmm::Constraint* constraint_read_  = nullptr; /* Constraint for maximum read bandwidth*/
  std::unordered_map<s4u::Disk::Operation, s4u::Disk::SharingPolicy> sharing_policy_ = {
      {s4u::Disk::Operation::READ, s4u::Disk::SharingPolicy::LINEAR},
      {s4u::Disk::Operation::WRITE, s4u::Disk::SharingPolicy::LINEAR},
      {s4u::Disk::Operation::READWRITE, s4u::Disk::SharingPolicy::LINEAR}};
  std::unordered_map<s4u::Disk::Operation, s4u::NonLinearResourceCb> sharing_policy_cb_ = {};

  void apply_sharing_policy_cfg();

protected:
  ~DiskImpl() override = default; // Disallow direct deletion. Call destroy() instead.

public:
  Metric read_bw_  = {0.0, 0, nullptr};
  Metric write_bw_ = {0.0, 0, nullptr};

  explicit DiskImpl(const std::string& name, double read_bandwidth, double write_bandwidth);
  DiskImpl(const DiskImpl&) = delete;
  DiskImpl& operator=(const DiskImpl&) = delete;

  /** @brief Public interface */
  const s4u::Disk* get_iface() const { return &piface_; }
  s4u::Disk* get_iface() { return &piface_; }
  DiskImpl* set_host(s4u::Host* host);
  s4u::Host* get_host() const { return host_; }

  virtual void set_read_bandwidth(double read_bw) = 0;
  double get_read_bandwidth() const { return read_bw_.peak * read_bw_.scale; }

  virtual void set_write_bandwidth(double write_bw) = 0;
  double get_write_bandwidth() const { return write_bw_.peak * write_bw_.scale; }

  DiskImpl* set_read_constraint(lmm::Constraint* constraint_read);
  lmm::Constraint* get_read_constraint() const { return constraint_read_; }

  DiskImpl* set_write_constraint(lmm::Constraint* constraint_write);
  lmm::Constraint* get_write_constraint() const { return constraint_write_; }

  DiskImpl* set_read_bandwidth_profile(profile::Profile* profile);
  DiskImpl* set_write_bandwidth_profile(profile::Profile* profile);

  void set_sharing_policy(s4u::Disk::Operation op, s4u::Disk::SharingPolicy policy, const s4u::NonLinearResourceCb& cb);
  s4u::Disk::SharingPolicy get_sharing_policy(s4u::Disk::Operation op) const;

  /** @brief Check if the Disk is used (if an action currently uses its resources) */
  bool is_used() const override;
  void turn_on() override;
  void turn_off() override;

  void seal() override;
  void destroy(); // Must be called instead of the destructor
};

/**********
 * Action *
 **********/

class DiskAction : public Action {
public:
  static xbt::signal<void(DiskAction const&, Action::State, Action::State)> on_state_change;

  using Action::Action;
  void set_state(simgrid::kernel::resource::Action::State state) override;

  double sharing_penalty_ = {};
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif /* DISK_INTERFACE_HPP_ */
