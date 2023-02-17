/* Copyright (c) 2013-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "src/kernel/resource/DiskImpl.hpp"

#ifndef DISK_S19_HPP_
#define DISK_S19_HPP_

namespace simgrid::kernel::resource {

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
  explicit DiskS19Model(const std::string& name);
  DiskS19Model(const DiskS19Model&)            = delete;
  DiskS19Model& operator=(const DiskS19Model&) = delete;
  DiskImpl* create_disk(const std::string& name, double read_bandwidth, double write_bandwidth) override;

  void update_actions_state(double now, double delta) override;
};

/************
 * Resource *
 ************/

class DiskS19 : public DiskImpl {
public:
  using DiskImpl::DiskImpl;
  void apply_event(kernel::profile::Event* triggered, double value) override;
  DiskAction* io_start(sg_size_t size, s4u::Io::OpType type) override;
};

/**********
 * Action *
 **********/

class DiskS19Action : public DiskAction {
public:
  DiskS19Action(Model* model, double cost, bool failed);
};

} // namespace simgrid::kernel::resource
#endif /* DISK_S19_HPP_ */
