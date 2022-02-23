/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MUTEX_OBSERVER_HPP
#define SIMGRID_MC_MUTEX_OBSERVER_HPP

#include "simgrid/forward.h"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"

#include <string>

namespace simgrid {
namespace kernel {
namespace actor {

/* All the observers of Mutex transitions are very similar, so implement them all together in this class */
class MutexObserver : public SimcallObserver {
  mc::Transition::Type type_;
  activity::MutexImpl* const mutex_ = nullptr;

public:
  MutexObserver(ActorImpl* actor, mc::Transition::Type type, activity::MutexImpl* mutex);

  void serialize(std::stringstream& stream) const override;
  activity::MutexImpl* get_mutex() const { return mutex_; }
  bool is_enabled() override;
};

} // namespace actor
} // namespace kernel
} // namespace simgrid

#endif
