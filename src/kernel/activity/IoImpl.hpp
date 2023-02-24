/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_IO_HPP
#define SIMGRID_KERNEL_ACTIVITY_IO_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include <simgrid/s4u/Io.hpp>

namespace simgrid::kernel::activity {

class XBT_PUBLIC IoImpl : public ActivityImpl_T<IoImpl> {
  s4u::Host* host_                    = nullptr;
  resource::DiskImpl* disk_           = nullptr;
  s4u::Host* dst_host_                = nullptr;
  resource::DiskImpl* dst_disk_       = nullptr;
  double sharing_penalty_             = 1.0;
  sg_size_t size_                     = 0;
  s4u::Io::OpType type_               = s4u::Io::OpType::READ;
  sg_size_t performed_ioops_          = 0;

public:
  IoImpl();

  IoImpl& set_sharing_penalty(double sharing_penalty);
  IoImpl& set_size(sg_size_t size);
  IoImpl& set_type(s4u::Io::OpType type);
  IoImpl& set_disk(resource::DiskImpl* disk);
  IoImpl& set_host(s4u::Host* host);
  s4u::Host* get_host() const { return host_; }
  IoImpl& set_dst_disk(resource::DiskImpl* disk);
  IoImpl& set_dst_host(s4u::Host* host);
  s4u::Host* get_dst_host() const { return dst_host_; }

  IoImpl& update_sharing_penalty(double sharing_penalty);

  sg_size_t get_performed_ioops() const { return performed_ioops_; }
  resource::DiskImpl* get_disk() const { return disk_; }

  IoImpl* start();
  void set_exception(actor::ActorImpl* issuer) override;
  void finish() override;
};
} // namespace simgrid::kernel::activity

#endif
