/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_IO_HPP
#define SIMGRID_KERNEL_ACTIVITY_IO_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.hpp"
#include <simgrid/s4u/Io.hpp>

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC IoImpl : public ActivityImpl_T<IoImpl> {
  resource::StorageImpl* storage_ = nullptr;
  resource::DiskImpl* disk_       = nullptr;
  sg_size_t size_                 = 0;
  s4u::Io::OpType type_           = s4u::Io::OpType::READ;
  sg_size_t performed_ioops_      = 0;

public:
  IoImpl& set_size(sg_size_t size);
  IoImpl& set_type(s4u::Io::OpType type);
  IoImpl& set_storage(resource::StorageImpl* storage);
  IoImpl& set_disk(resource::DiskImpl* disk);

  sg_size_t get_performed_ioops() { return performed_ioops_; }

  IoImpl* start();
  void post() override;
  void finish() override;

  static xbt::signal<void(IoImpl const&)> on_start;
  static xbt::signal<void(IoImpl const&)> on_completion;
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif
