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

XBT_PUBLIC_DATA simgrid::kernel::resource::DiskModel* surf_disk_model;

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
  DiskModel();
  DiskModel(const DiskModel&) = delete;
  DiskModel& operator=(const DiskModel&) = delete;
  ~DiskModel() override;

  virtual DiskImpl* createDisk(const std::string& id, double read_bw, double write_bw) = 0;
};

/************
 * Resource *
 ************/
class DiskImpl : public Resource, public xbt::PropertyHolder {
  bool currently_destroying_ = false;
  s4u::Host* host_           = nullptr;
  s4u::Disk piface_;
  double read_bw_;
  double write_bw_;
  lmm::Constraint* constraint_write_; /* Constraint for maximum write bandwidth*/
  lmm::Constraint* constraint_read_;  /* Constraint for maximum read bandwidth*/

public:
  DiskImpl(Model* model, const std::string& name, kernel::lmm::System* maxmin_system, double read_bw, double bwrite_bw);
  DiskImpl(const DiskImpl&) = delete;
  DiskImpl& operator=(const DiskImpl&) = delete;

  ~DiskImpl() override;

  /** @brief Public interface */
  const s4u::Disk* get_iface() const { return &piface_; }
  s4u::Disk* get_iface() { return &piface_; }
  s4u::Host* get_host() const { return host_; }
  void set_host(s4u::Host* host) { host_ = host; }

  double get_read_bandwidth() const { return read_bw_; }
  double get_write_bandwidth() const { return write_bw_; }
  lmm::Constraint* get_read_constraint() const { return constraint_read_; }
  lmm::Constraint* get_write_constraint() const { return constraint_write_; }

  /** @brief Check if the Disk is used (if an action currently uses its resources) */
  bool is_used() const override;
  void apply_event(profile::Event* event, double value) override;
  void turn_on() override;
  void turn_off() override;

  void destroy(); // Must be called instead of the destructor
  virtual DiskAction* io_start(sg_size_t size, s4u::Io::OpType type) = 0;
  virtual DiskAction* read(sg_size_t size)                           = 0;
  virtual DiskAction* write(sg_size_t size)                          = 0;
};

/**********
 * Action *
 **********/

class DiskAction : public Action {
public:
  static xbt::signal<void(DiskAction const&, Action::State, Action::State)> on_state_change;

  DiskAction(Model* model, double cost, bool failed, DiskImpl* disk, s4u::Io::OpType type)
      : Action(model, cost, failed), type_(type), disk_(disk){};

  /**
   * @brief diskAction constructor
   *
   * @param model The DiskModel associated to this DiskAction
   * @param cost The cost of this DiskAction in bytes
   * @param failed [description]
   * @param var The lmm variable associated to this DiskAction if it is part of a LMM component
   * @param disk The Disk associated to this DiskAction
   * @param type [description]
   */
  DiskAction(kernel::resource::Model* model, double cost, bool failed, kernel::lmm::Variable* var, DiskImpl* disk,
             s4u::Io::OpType type)
      : Action(model, cost, failed, var), type_(type), disk_(disk){};

  void set_state(simgrid::kernel::resource::Action::State state) override;

  s4u::Io::OpType type_;
  DiskImpl* disk_;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif /* DISK_INTERFACE_HPP_ */
