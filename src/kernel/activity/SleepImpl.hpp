/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_SLEEP_HPP
#define SIMIX_SYNCHRO_SLEEP_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC SleepImpl : public ActivityImpl {
  ~SleepImpl() override;

public:
  explicit SleepImpl(const std::string& name, s4u::Host* host) : ActivityImpl(name), host_(host) {}
  void post() override;
  void finish() override;
  SleepImpl* start(double duration);

  sg_host_t host_ = nullptr;
};
}
}
} // namespace simgrid::kernel::activity

#endif
