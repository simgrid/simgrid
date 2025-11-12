/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/modelchecker.h>
#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Engine.hpp>

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/WaitTestObserver.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/mc/mc_replay.hpp"

#include <boost/range/algorithm.hpp>
#include <cmath> // isfinite()

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_activity, kernel, "Kernel activity-related synchronization");

namespace simgrid::kernel::activity {

ActivityImpl::~ActivityImpl()
{
  clean_action();
  XBT_DEBUG("Destroy activity %p", this);
}

void ActivityImpl::register_simcall(actor::Simcall* simcall)
{
  simcalls_.push_back(simcall);
  simcall->issuer_->waiting_synchros_.push_back(this);
}

void ActivityImpl::unregister_simcall(actor::Simcall* simcall)
{
  // Remove the first occurrence of simcall:
  auto j = boost::range::find(simcalls_, simcall);
  if (j != simcalls_.end())
    simcalls_.erase(j);

  auto s = boost::range::find(simcall->issuer_->waiting_synchros_, this);
  if (s != simcall->issuer_->waiting_synchros_.end())
    simcall->issuer_->waiting_synchros_.erase(s);
}

actor::ActorImpl* ActivityImpl::unregister_first_simcall()
{
  actor::Simcall* simcall = simcalls_.front();
  simcalls_.pop_front();

  auto s = boost::range::find(simcall->issuer_->waiting_synchros_, this);
  if (s != simcall->issuer_->waiting_synchros_.end())
    simcall->issuer_->waiting_synchros_.erase(s);

  if (simcall->timeout_cb_) { // The timeout, if any, is now useless as the activity finished
    simcall->timeout_cb_->remove();
    simcall->timeout_cb_ = nullptr;
  }

  /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
   * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
   * simcall */
  if (auto* observer = dynamic_cast<actor::ActivityWaitanySimcall*>(simcall->observer_)) {
    auto activities = observer->get_activities();
    for (auto* act : activities)
      act->unregister_simcall(simcall);

    if (not MC_is_active() && not MC_record_replay_is_active()) {
      auto element = std::find(activities.begin(), activities.end(), this);
      int rank     = element != activities.end() ? static_cast<int>(std::distance(activities.begin(), element)) : -1;
      observer->set_result(rank);
    }
  }

  /* The actor is not blocked in a simcall. It's probably exiting and called finish() itself. Don't notify it. */
  if (simcall->call_ == actor::Simcall::Type::NONE)
    return nullptr;

  auto issuer = simcall->issuer_;
  if (not issuer->get_host()->is_on())
    issuer->set_wannadie();
  if (issuer->wannadie()) // Do not answer to dying actors
    return nullptr;

  return issuer;
}

void ActivityImpl::clean_action()
{
  if (model_action_) {
    model_action_->unref();
    model_action_ = nullptr;
  }
}

double ActivityImpl::get_remaining() const
{
  return model_action_ ? model_action_->get_remains() : 0;
}

const char* ActivityImpl::get_state_str() const
{
  return to_c_str(state_);
}

void ActivityImpl::detach()
{
  set_detached(true);
  EngineImpl::get_instance()->get_maestro()->activities_.insert(this);
}

bool ActivityImpl::test(actor::ActorImpl* issuer)
{
  if (state_ != State::WAITING && state_ != State::RUNNING) {
    finish();
    issuer->exception_ = nullptr; // Do not propagate exception in that case
    return true;
  }

  if (auto* observer = dynamic_cast<kernel::actor::ActivityTestSimcall*>(issuer->simcall_.observer_))
    observer->set_result(false);

  return false;
}

ssize_t ActivityImpl::test_any(actor::ActorImpl* issuer, const std::vector<ActivityImpl*>& activities)
{
  auto* observer = dynamic_cast<kernel::actor::ActivityTestanySimcall*>(issuer->simcall_.observer_);
  xbt_assert(observer != nullptr);

  if (MC_is_active() || MC_record_replay_is_active()) {
    int idx = observer->get_value();
    xbt_assert(idx == -1 || activities[idx]->test(issuer));
    return idx;
  }

  for (std::size_t i = 0; i < activities.size(); ++i) {
    if (activities[i]->test(issuer)) {
      observer->set_result(i);
      return i;
    }
  }
  return -1;
}

void ActivityImpl::wait_for(actor::ActorImpl* issuer, double timeout)
{
  XBT_DEBUG("Wait for execution of synchro %p, state %s", this, get_state_str());
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");

  /* Associate this simcall to the synchro */
  register_simcall(&issuer->simcall_);

  /* If the synchro is already finished then perform the error handling */
  if (state_ != State::WAITING && state_ != State::RUNNING) {
    finish();
  } else if (timeout >= 0.) {
    xbt_assert(not MC_is_active() && not MC_record_replay_is_active(), "MC does currently not support timeouts.");

    issuer->simcall_.timeout_cb_ = timer::Timer::set(s4u::Engine::get_clock() + timeout, [this, issuer]() {
      issuer->simcall_.timeout_cb_ = nullptr;
      if (model_action_ && (model_action_->get_state() == resource::Action::State::FINISHED ||
                            model_action_->get_state() == resource::Action::State::FAILED))
        return; // The activity terminated or failed right on time; do not timeout

      this->unregister_simcall(&issuer->simcall_);
      issuer->exception_ = nullptr;
      auto* observer     = dynamic_cast<kernel::actor::ActivityWaitSimcall*>(issuer->simcall_.observer_);
      xbt_assert(observer != nullptr);
      observer->set_result(true); // Returns that the wait_for timeouted
      issuer->simcall_answer();
    });
  }
}

void ActivityImpl::wait_any_for(actor::ActorImpl* issuer, const std::vector<ActivityImpl*>& activities, double timeout)
{
  XBT_DEBUG("Wait for execution of any synchro");
  if (MC_is_active() || MC_record_replay_is_active()) {
    auto* observer = dynamic_cast<kernel::actor::ActivityWaitanySimcall*>(issuer->simcall_.observer_);
    xbt_assert(observer != nullptr);
    xbt_assert(timeout <= 0.0, "Timeout not implemented for waitany in the model-checker");
    if (int idx = observer->get_value(); idx != -1) {
      auto* act = activities.at(idx);
      act->register_simcall(&issuer->simcall_);
      observer->set_result(idx);
      act->set_state(State::DONE);
      act->finish();
    }
    return;
  }

  if (timeout < 0.0) {
    issuer->simcall_.timeout_cb_ = nullptr;
  } else {
    issuer->simcall_.timeout_cb_ = timer::Timer::set(s4u::Engine::get_clock() + timeout, [issuer, &activities]() {
      issuer->simcall_.timeout_cb_ = nullptr;
      for (auto* act : activities)
        act->unregister_simcall(&issuer->simcall_);
      // default result (-1) is set in actor::ActivityWaitanySimcall
      issuer->simcall_answer();
    });
  }

  for (auto* act : activities) {
    /* associate this simcall to the the synchro */
    act->register_simcall(&issuer->simcall_);
    /* see if the synchro is already finished */
    if (act->get_state() != State::WAITING && act->get_state() != State::RUNNING) {
      act->finish();
      break;
    }
  }
  XBT_DEBUG("Exit from ActivityImlp::wait_any_for");
}

void ActivityImpl::suspend()
{
  XBT_VERB("This activity is suspended (remain: %f)", model_action_->get_remains());
  get_iface()->fire_on_suspend();
  get_iface()->fire_on_this_suspend();
  model_action_->suspend();
}

void ActivityImpl::resume()
{
  if (model_action_ == nullptr)
    return;
  XBT_VERB("This activity is resumed (remain: %f)", model_action_->get_remains());
  get_iface()->fire_on_resume();
  get_iface()->fire_on_this_resume();
  model_action_->resume();
}

void ActivityImpl::cancel()
{
  XBT_VERB("Activity %p is canceled", this);
  if (model_action_ != nullptr)
    model_action_->cancel();
  set_state(State::CANCELED);
  if (actor_)
    actor_->activities_.erase(this);
}

// boost::intrusive_ptr<Activity> support:
void intrusive_ptr_add_ref(ActivityImpl* activity)
{
  activity->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(ActivityImpl* activity)
{
  if (activity->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete activity;
  }
}

} // namespace simgrid::kernel::activity
