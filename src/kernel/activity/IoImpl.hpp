/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_IO_HPP
#define SIMGRID_KERNEL_ACTIVITY_IO_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include <simgrid/s4u/Io.hpp>

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC IoImpl : public ActivityImpl_T<IoImpl> {
  resource::DiskImpl* disk_           = nullptr;
  double sharing_penalty_             = 1.0;
  sg_size_t size_                     = 0;
  s4u::Io::OpType type_               = s4u::Io::OpType::READ;
  sg_size_t performed_ioops_          = 0;
  resource::Action* timeout_detector_ = nullptr;
public:
  IoImpl();

  IoImpl& set_sharing_penalty(double sharing_penalty);
  IoImpl& set_timeout(double timeout) override;
  IoImpl& set_size(sg_size_t size);
  IoImpl& set_type(s4u::Io::OpType type);
  IoImpl& set_disk(resource::DiskImpl* disk);

  IoImpl& update_sharing_penalty(double sharing_penalty);

  sg_size_t get_performed_ioops() const { return performed_ioops_; }
  resource::DiskImpl* get_disk() const { return disk_; }

  IoImpl* start();
  void post() override;
  void set_exception(actor::ActorImpl* issuer) override;
  void finish() override;
  static void wait_any_for(actor::ActorImpl* issuer, const std::vector<IoImpl*>& ios, double timeout);
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif
