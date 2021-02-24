/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ActivityImpl.hpp"
#include "simgrid/modelchecker.h"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_private.hpp"
#include <cmath> // isfinite()

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);

namespace simgrid {
namespace kernel {
namespace activity {

ActivityImpl::~ActivityImpl()
{
  clean_action();
  XBT_DEBUG("Destroy activity %p", this);
}

void ActivityImpl::register_simcall(smx_simcall_t simcall)
{
  simcalls_.push_back(simcall);
  simcall->issuer_->waiting_synchro_ = this;
}

void ActivityImpl::clean_action()
{
  if (surf_action_) {
    surf_action_->unref();
    surf_action_ = nullptr;
  }
}

double ActivityImpl::get_remaining() const
{
  return surf_action_ ? surf_action_->get_remains() : 0;
}

const char* ActivityImpl::get_state_str() const
{
  return to_c_str(state_);
}

bool ActivityImpl::test()
{
  if (state_ != State::WAITING && state_ != State::RUNNING) {
    finish();
    return true;
  }
  return false;
}

void ActivityImpl::wait_for(actor::ActorImpl* issuer, double timeout)
{
  XBT_DEBUG("Wait for execution of synchro %p, state %d", this, (int)state_);
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");

  /* Associate this simcall to the synchro */
  register_simcall(&issuer->simcall_);

  if (MC_is_active() || MC_record_replay_is_active()) {
    int idx = SIMCALL_GET_MC_VALUE(issuer->simcall_);
    if (idx == 0) {
      state_ = simgrid::kernel::activity::State::DONE;
    } else {
      /* If we reached this point, the wait simcall must have a timeout */
      /* Otherwise it shouldn't be enabled and executed by the MC */
      if (timeout < 0.0)
        THROW_IMPOSSIBLE;
      state_ = simgrid::kernel::activity::State::TIMEOUT;
    }
    finish();
    return;
  }

  /* If the synchro is already finished then perform the error handling */
  if (state_ != simgrid::kernel::activity::State::RUNNING)
    finish();
  else if (timeout == 0.) {
    // still running and timeout == 0 ? We need to report a timeout
    state_ = simgrid::kernel::activity::State::TIMEOUT;
    finish();
  } else {
    /* we need a sleep action (even when the timeout is infinite) to be notified of host failures */
    set_timeout(timeout);
  }
}

void ActivityImpl::suspend()
{
  if (surf_action_ == nullptr)
    return;
  XBT_VERB("This activity is suspended (remain: %f)", surf_action_->get_remains());
  surf_action_->suspend();
  on_suspended(*this);
}

void ActivityImpl::resume()
{
  if (surf_action_ == nullptr)
    return;
  XBT_VERB("This activity is resumed (remain: %f)", surf_action_->get_remains());
  surf_action_->resume();
  on_resumed(*this);
}

void ActivityImpl::cancel()
{
  XBT_VERB("Activity %p is canceled", this);
  if (surf_action_ != nullptr)
    surf_action_->cancel();
  state_ = State::CANCELED;
}

// boost::intrusive_ptr<Activity> support:
void intrusive_ptr_add_ref(simgrid::kernel::activity::ActivityImpl* activity)
{
  activity->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(simgrid::kernel::activity::ActivityImpl* activity)
{
  if (activity->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete activity;
  }
}
xbt::signal<void(ActivityImpl const&)> ActivityImpl::on_resumed;
xbt::signal<void(ActivityImpl const&)> ActivityImpl::on_suspended;
}
}
} // namespace simgrid::kernel::activity::
