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

/* abstract */
class MutexObserver : public SimcallObserver {
  activity::MutexImpl* const mutex_;

public:
  MutexObserver(ActorImpl* actor, activity::MutexImpl* mutex);
  virtual ~MutexObserver() = default;
  activity::MutexImpl* get_mutex() const { return mutex_; }
};

class MutexTestObserver : public MutexObserver {
public:
  MutexTestObserver(ActorImpl* actor, activity::MutexImpl* mutex);
};

class MutexLockAsyncObserver : public MutexObserver {
public:
  MutexLockAsyncObserver(ActorImpl* actor, activity::MutexImpl* mutex);
};

class MutexLockWaitObserver : public MutexObserver {
  activity::MutexAcquisitionImplPtr synchro_;

public:
  MutexLockWaitObserver(ActorImpl* actor, activity::MutexAcquisitionImplPtr synchro);
  bool is_enabled() override;
};

class MutexUnlockObserver : public MutexObserver {
  using MutexObserver::MutexObserver;
};

} // namespace actor
} // namespace kernel
} // namespace simgrid

#endif
