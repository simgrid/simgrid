/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Io.hpp>
#include <xbt/log.h>

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_activity, s4u, "S4U activities");

namespace simgrid {

template class xbt::Extendable<s4u::Activity>;

namespace s4u {

xbt::signal<void(Activity&)> Activity::on_veto;
xbt::signal<void(Activity&)> Activity::on_completion;

std::set<Activity*>* Activity::vetoed_activities_ = nullptr;

void Activity::destroy()
{
  /* First Remove all dependencies */
  while (not dependencies_.empty())
    (*(dependencies_.begin()))->remove_successor(this);
  while (not successors_.empty())
    this->remove_successor(successors_.front());

  cancel();
}

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
    if (dynamic_cast<Comm*>(this))
      throw NetworkFailureException(XBT_THROW_POINT, "Cannot wait for a failed comm");
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

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::ActivityTestSimcall observer{issuer, pimpl_.get()};
  if (kernel::actor::simcall([&observer] { return observer.get_activity()->test(observer.get_issuer()); }, &observer)) {
    complete(State::FINISHED);
    return true;
  }
  return false;
}

ssize_t Activity::test_any(const std::vector<ActivityPtr>& activities)
{
  std::vector<kernel::activity::ActivityImpl*> ractivities(activities.size());
  std::transform(begin(activities), end(activities), begin(ractivities),
                 [](const ActivityPtr& act) { return act->pimpl_.get(); });

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::ActivityTestanySimcall observer{issuer, ractivities};
  ssize_t changed_pos = kernel::actor::simcall(
      [&observer] {
        return kernel::activity::ActivityImpl::test_any(observer.get_issuer(), observer.get_activities());
      },
      &observer);
  if (changed_pos != -1)
    activities.at(changed_pos)->complete(State::FINISHED);
  return changed_pos;
}

ssize_t Activity::wait_any_for(const std::vector<ActivityPtr>& activities, double timeout)
{
  std::vector<kernel::activity::ActivityImpl*> ractivities(activities.size());
  std::transform(begin(activities), end(activities), begin(ractivities),
                 [](const ActivityPtr& activity) { return activity->pimpl_.get(); });

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::ActivityWaitanySimcall observer{issuer, ractivities, timeout};
  ssize_t changed_pos = kernel::actor::simcall_blocking(
      [&observer] {
        kernel::activity::ActivityImpl::wait_any_for(observer.get_issuer(), observer.get_activities(),
                                                     observer.get_timeout());
      },
      &observer);
  if (changed_pos != -1)
    activities.at(changed_pos)->complete(State::FINISHED);
  return changed_pos;
}

Activity* Activity::cancel()
{
  kernel::actor::simcall([this] {
    XBT_HERE();
    if (pimpl_)
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
double Activity::get_start_time() const
{
  return pimpl_->get_start_time();
}
double Activity::get_finish_time() const
{
  return pimpl_->get_finish_time();
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
