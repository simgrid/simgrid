/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_SYNCHRO_RAW_HPP
#define SIMGRID_KERNEL_ACTIVITY_SYNCHRO_RAW_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

  /** Used to implement mutexes, semaphores and conditions */
class XBT_PUBLIC RawImpl : public ActivityImpl_T<RawImpl> {
  sg_host_t host_ = nullptr;
  double timeout_ = -1;

public:
  RawImpl& set_host(s4u::Host* host);
  RawImpl& set_timeout(double timeout) override;

  RawImpl* start();
  void suspend() override;
  void resume() override;
  void cancel() override;
  void post() override;
  void finish() override;
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif
