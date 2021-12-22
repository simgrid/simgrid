/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Io.hpp>
#include <xbt/log.h>

#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_activity, s4u, "S4U activities");

namespace simgrid {
namespace s4u {

xbt::signal<void(Activity&)> Activity::on_veto;
xbt::signal<void(Activity&)> Activity::on_completion;

std::set<Activity*>* Activity::vetoed_activities_ = nullptr;

void Activity::wait_until(double time_limit)
{
  double now = Engine::get_clock();
  if (time_limit > now)
    wait_for(time_limit - now);
}

Activity* Activity::wait_for(double timeout)
{
  if (state_ == State::INITED)
    vetoable_start();

  if (state_ == State::FAILED) {
    if (dynamic_cast<Exec*>(this))
      throw HostFailureException(XBT_THROW_POINT, "Cannot wait for a failed exec");
    if (dynamic_cast<Io*>(this))
      throw StorageFailureException(XBT_THROW_POINT, "Cannot wait for a failed I/O");
  }

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::ActivityWaitSimcall observer{issuer, pimpl_.get(), timeout};
  if (kernel::actor::simcall_blocking(
          [&observer] { observer.get_activity()->wait_for(observer.get_issuer(), observer.get_timeout()); }, &observer))
    throw TimeoutException(XBT_THROW_POINT, "Timeouted");
  complete(State::FINISHED);
  return this;
}

bool Activity::test()
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED || state_ == State::STARTING ||
             state_ == State::CANCELED || state_ == State::FINISHED);

  if (state_ == State::CANCELED || state_ == State::FINISHED)
    return true;

  if (state_ == State::INITED || state_ == State::STARTING)
    this->vetoable_start();

  if (kernel::actor::simcall([this] { return this->get_impl()->test(); })) {
    complete(State::FINISHED);
    return true;
  }

  return false;
}

Activity* Activity::cancel()
{
  kernel::actor::simcall([this] {
    XBT_HERE();
    pimpl_->cancel();
  });
  complete(State::CANCELED);
  return this;
}

Activity* Activity::suspend()
{
  if (suspended_)
    return this; // Already suspended
  suspended_ = true;

  if (state_ == State::STARTED)
    pimpl_->suspend();

  return this;
}

Activity* Activity::resume()
{
  if (not suspended_)
    return this; // nothing to restore when it's not suspended

  if (state_ == State::STARTED)
    pimpl_->resume();

  return this;
}

const char* Activity::get_state_str() const
{
  return to_c_str(state_);
}

double Activity::get_remaining() const
{
  if (state_ == State::INITED || state_ == State::STARTING)
    return remains_;
  else
    return pimpl_->get_remaining();
}

Activity* Activity::set_remaining(double remains)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the remaining amount of work once the Activity is started");
  remains_ = remains;
  return this;
}

} // namespace s4u
} // namespace simgrid
