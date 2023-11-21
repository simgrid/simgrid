/* Copyright (c) 2023-. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/CommObserver.hpp"
#include <simgrid/Exception.hpp>
#include <simgrid/activity_set.h>
#include <simgrid/s4u/ActivitySet.hpp>
#include <simgrid/s4u/Engine.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_activityset, s4u_activity, "S4U set of activities");

namespace simgrid {

template class xbt::Extendable<s4u::ActivitySet>;

namespace s4u {

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

void ActivitySet::handle_failed_activities()
{
  for (size_t i = 0; i < activities_.size(); i++) {
    auto act = activities_[i];
    if (act->pimpl_->get_state() == kernel::activity::State::FAILED) {
      act->complete(Activity::State::FAILED);

      failed_activities_.push_back(act);
      activities_[i] = activities_[activities_.size() - 1];
      activities_.resize(activities_.size() - 1);
      i--; // compensate the i++ occuring at the end of the loop
    }
  }
}

ActivityPtr ActivitySet::wait_any_for(double timeout)
{
  std::vector<kernel::activity::ActivityImpl*> act_impls(activities_.size());
  std::transform(begin(activities_), end(activities_), begin(act_impls),
                 [](const ActivityPtr& activity) { return activity->pimpl_.get(); });

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::ActivityWaitanySimcall observer{issuer, act_impls, timeout, "wait_any_for"};
  try {
    ssize_t changed_pos = kernel::actor::simcall_blocking(
        [&observer] {
          kernel::activity::ActivityImpl::wait_any_for(observer.get_issuer(), observer.get_activities(),
                                                       observer.get_timeout());
        },
        &observer);
    if (changed_pos == -1)
      throw TimeoutException(XBT_THROW_POINT, "Timeouted");

    auto ret = activities_.at(changed_pos);
    erase(ret);
    ret->complete(Activity::State::FINISHED);
    return ret;
  } catch (const HostFailureException& e) {
    handle_failed_activities();
    throw;
  } catch (const NetworkFailureException& e) {
    handle_failed_activities();
    throw;
  } catch (const StorageFailureException& e) {
    handle_failed_activities();
    throw;
  }
}

ActivityPtr ActivitySet::get_failed_activity()
{
  if (failed_activities_.empty())
    return ActivityPtr(nullptr);
  auto ret = failed_activities_.back();
  failed_activities_.pop_back();
  return ret;
}

} // namespace s4u
} // namespace simgrid

SG_BEGIN_DECL

sg_activity_set_t sg_activity_set_init()
{
  return new simgrid::s4u::ActivitySet();
}
void sg_activity_set_push(sg_activity_set_t as, sg_activity_t acti)
{
  as->push(acti);
}
void sg_activity_set_erase(sg_activity_set_t as, sg_activity_t acti)
{
  as->erase(acti);
}
size_t sg_activity_set_size(sg_activity_set_t as)
{
  return as->size();
}
int sg_activity_set_empty(sg_activity_set_t as)
{
  return as->empty();
}

sg_activity_t sg_activity_set_test_any(sg_activity_set_t as)
{
  return as->test_any().get();
}
void sg_activity_set_wait_all(sg_activity_set_t as)
{
  as->wait_all();
}
int sg_activity_set_wait_all_for(sg_activity_set_t as, double timeout)
{
  try {
    as->wait_all_for(timeout);
    return 1;
  } catch (const simgrid::TimeoutException& e) {
    return 0;
  }
}
sg_activity_t sg_activity_set_wait_any(sg_activity_set_t as)
{
  return as->wait_any().get();
}
sg_activity_t sg_activity_set_wait_any_for(sg_activity_set_t as, double timeout)
{
  try {
    return as->wait_any_for(timeout).get();
  } catch (const simgrid::TimeoutException& e) {
    return nullptr;
  }
}

void sg_activity_set_delete(sg_activity_set_t as)
{
  delete as;
}
void sg_activity_unref(sg_activity_t acti)
{
  acti->unref();
}

SG_END_DECL
