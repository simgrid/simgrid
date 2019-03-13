/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_IO_HPP
#define SIMIX_SYNCHRO_IO_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.hpp"
#include <simgrid/s4u/Io.hpp>

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC IoImpl : public ActivityImpl {
public:
  ~IoImpl() override;
  explicit IoImpl(const std::string& name, resource::StorageImpl* storage);

  IoImpl* start(sg_size_t size, s4u::Io::OpType type);
  void post() override;
  void finish() override;
  void cancel();
  double get_remaining();
  sg_size_t get_performed_ioops() { return performed_ioops_; }

  resource::StorageImpl* storage_                 = nullptr;
  sg_size_t performed_ioops_                      = 0;
  static xbt::signal<void(IoImplPtr)> on_start;
  static xbt::signal<void(IoImplPtr)> on_completion;
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif
