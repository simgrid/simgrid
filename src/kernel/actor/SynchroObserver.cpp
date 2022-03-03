/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/SynchroObserver.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/mc_config.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(obs_mutex, mc_observer, "Logging specific to mutex simcalls observation");

namespace simgrid {
namespace kernel {
namespace actor {

MutexObserver::MutexObserver(ActorImpl* actor, mc::Transition::Type type, activity::MutexImpl* mutex)
    : SimcallObserver(actor), type_(type), mutex_(mutex)
{
  xbt_assert(mutex_);
}

void MutexObserver::serialize(std::stringstream& stream) const
{
  const auto* owner = get_mutex()->get_owner();
  stream << (short)type_ << ' ' << get_mutex()->get_id() << ' ' << (owner != nullptr ? owner->get_pid() : -1);
}

bool MutexObserver::is_enabled()
{
  // Only wait can be disabled
  return type_ != mc::Transition::Type::MUTEX_WAIT || mutex_->get_owner() == get_issuer();
}

SemaphoreObserver::SemaphoreObserver(ActorImpl* actor, mc::Transition::Type type, activity::SemaphoreImpl* sem)
    : SimcallObserver(actor), type_(type), sem_(sem)
{
  xbt_assert(sem_);
}

void SemaphoreObserver::serialize(std::stringstream& stream) const
{
  stream << (short)type_ << ' ' << get_sem()->get_id() << ' ' << false /* Granted is ignored for LOCK/UNLOCK */;
}

SemaphoreAcquisitionObserver::SemaphoreAcquisitionObserver(ActorImpl* actor, mc::Transition::Type type,
                                                           activity::SemAcquisitionImpl* acqui, double timeout)
    : ResultingSimcall(actor, false), type_(type), acquisition_(acqui), timeout_(timeout)
{
}
bool SemaphoreAcquisitionObserver::is_enabled()
{
  return acquisition_->granted_;
}
void SemaphoreAcquisitionObserver::serialize(std::stringstream& stream) const
{
  stream << (short)type_ << ' ' << acquisition_->semaphore_->get_id() << ' ' << acquisition_->granted_;
}

} // namespace actor
} // namespace kernel
} // namespace simgrid
