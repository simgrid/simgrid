/* Copyright (c) 2023-. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/CommObserver.hpp"
#include <simgrid/s4u/ActivitySet.hpp>
#include <simgrid/s4u/Engine.hpp>

namespace simgrid::s4u {

void ActivitySet::erase(ActivityPtr a)
{
  for (auto it = activities_.begin(); it != activities_.end(); it++)
    if (*it == a) {
      activities_.erase(it);
      return;
    }
}

void ActivitySet::wait_all_for(double timeout)
{
  if (timeout < 0.0) {
    for (const auto& act : activities_)
      act->wait();

  } else {

    double deadline = Engine::get_clock() + timeout;
    for (const auto& act : activities_)
      act->wait_until(deadline);
  }
  activities_.clear();
}

ActivityPtr ActivitySet::test_any()
{
  std::vector<kernel::activity::ActivityImpl*> act_impls(activities_.size());
  std::transform(begin(activities_), end(activities_), begin(act_impls),
                 [](const ActivityPtr& act) { return act->pimpl_.get(); });

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::ActivityTestanySimcall observer{issuer, act_impls, "test_any"};
  ssize_t changed_pos = kernel::actor::simcall_answered(
      [&observer] {
        return kernel::activity::ActivityImpl::test_any(observer.get_issuer(), observer.get_activities());
      },
      &observer);
  if (changed_pos == -1)
    return ActivityPtr(nullptr);

  auto ret = activities_.at(changed_pos);
  erase(ret);
  ret->complete(Activity::State::FINISHED);
  return ret;
}

ActivityPtr ActivitySet::wait_any_for(double timeout)
{
  std::vector<kernel::activity::ActivityImpl*> act_impls(activities_.size());
  std::transform(begin(activities_), end(activities_), begin(act_impls),
                 [](const ActivityPtr& activity) { return activity->pimpl_.get(); });

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::ActivityWaitanySimcall observer{issuer, act_impls, timeout, "wait_any_for"};
  ssize_t changed_pos = kernel::actor::simcall_blocking(
      [&observer] {
        kernel::activity::ActivityImpl::wait_any_for(observer.get_issuer(), observer.get_activities(),
                                                     observer.get_timeout());
      },
      &observer);
  xbt_assert(changed_pos != -1,
             "ActivityImpl::wait_any_for is not supposed to return -1 but instead to raise exceptions");

  auto ret = activities_.at(changed_pos);
  erase(ret);
  ret->complete(Activity::State::FINISHED);
  return ret;
}

ActivityPtr ActivitySet::get_failed_activity()
{
  if (failed_activities_.empty())
    return ActivityPtr(nullptr);
  auto ret = failed_activities_.back();
  failed_activities_.pop_back();
  return ret;
}

}; // namespace simgrid::s4u