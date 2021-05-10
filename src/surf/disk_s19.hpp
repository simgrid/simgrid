/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "src/kernel/resource/DiskImpl.hpp"

#ifndef DISK_S19_HPP_
#define DISK_S19_HPP_

namespace simgrid {
namespace kernel {
namespace resource {

/***********
 * Classes *
 ***********/

class XBT_PRIVATE DiskS19Model;
class XBT_PRIVATE DiskS19;
class XBT_PRIVATE DiskS19Action;

/*********
 * Model *
 *********/

class DiskS19Model : public DiskModel {
public:
  using DiskModel::DiskModel;
  DiskImpl* create_disk(const std::string& name, double read_bandwidth, double write_bandwidth) override;

  DiskAction* io_start(const DiskImpl* disk, sg_size_t size, s4u::Io::OpType type) override;

  void update_actions_state(double now, double delta) override;
};

/************
 * Resource *
 ************/

class DiskS19 : public DiskImpl {
  void update_penalties(double delta);

public:
  using DiskImpl::DiskImpl;
  void set_read_bandwidth(double value) override;
  void set_write_bandwidth(double value) override;
  void apply_event(kernel::profile::Event* triggered, double value) override;
};

/**********
 * Action *
 **********/

class DiskS19Action : public DiskAction {
public:
  DiskS19Action(Model* model, double cost, bool failed);
  void update_remains_lazy(double now) override;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif /* DISK_S19_HPP_ */
