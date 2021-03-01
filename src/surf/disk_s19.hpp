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
  DiskS19Model();
  DiskImpl* create_disk(const std::string& name, double read_bandwidth, double write_bandwidth) override;
  double next_occurring_event(double now) override;
  void update_actions_state(double now, double delta) override;
};

/************
 * Resource *
 ************/

class DiskS19 : public DiskImpl {
public:
  explicit DiskS19(const std::string& name, double read_bw, double write_bw): DiskImpl(name, read_bw, write_bw) {}
  DiskAction* io_start(sg_size_t size, s4u::Io::OpType type) override;
  DiskAction* read(sg_size_t size) override;
  DiskAction* write(sg_size_t size) override;
};

/**********
 * Action *
 **********/

class DiskS19Action : public DiskAction {
public:
  DiskS19Action(Model* model, double cost, bool failed, DiskImpl* disk, s4u::Io::OpType type);
  void suspend() override;
  void cancel() override;
  void resume() override;
  void set_max_duration(double duration) override;
  void set_sharing_penalty(double sharing_penalty) override;
  void update_remains_lazy(double now) override;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif /* DISK_S19_HPP_ */
