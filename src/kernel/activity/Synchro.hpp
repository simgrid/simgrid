/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_SYNCHRO_RAW_HPP
#define SIMGRID_KERNEL_ACTIVITY_SYNCHRO_RAW_HPP

#include "src/kernel/activity/ActivityImpl.hpp"

#include <functional>

namespace simgrid {
namespace kernel {
namespace activity {

/** Used to implement mutexes, semaphores and conditions */
class XBT_PUBLIC SynchroImpl : public ActivityImpl_T<SynchroImpl> {
  sg_host_t host_ = nullptr;
  double timeout_ = -1;
  std::function<void()> finish_callback_;

public:
  explicit SynchroImpl(const std::function<void()>& finish_callback) : finish_callback_(finish_callback) {}
  SynchroImpl& set_host(s4u::Host* host);
  SynchroImpl& set_timeout(double timeout) override;

  SynchroImpl* start();
  void suspend() override;
  void resume() override;
  void cancel() override;
  void post() override;
  void set_exception(actor::ActorImpl* issuer) override;
  void finish() override;
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif
