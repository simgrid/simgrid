/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/MutexObserver.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
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
}

void MutexObserver::serialize(std::stringstream& stream) const
{
  auto* owner = get_mutex()->get_owner();
  stream << (short)type_ << ' ' << (uintptr_t)get_mutex() << ' ' << (owner != nullptr ? owner->get_pid() : -1);
}

bool MutexObserver::is_enabled()
{
  // Only wait can be disabled
  return type_ != mc::Transition::Type::MUTEX_WAIT || mutex_->get_owner() == get_issuer();
}

} // namespace actor
} // namespace kernel
} // namespace simgrid
