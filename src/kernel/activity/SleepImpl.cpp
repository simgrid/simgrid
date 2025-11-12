/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Host.hpp>

#include "src/kernel/activity/SleepImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/resource/CpuImpl.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_actor);

namespace simgrid::kernel::activity {

SleepImpl& SleepImpl::set_host(s4u::Host* host)
{
  host_ = host;
  return *this;
}

SleepImpl& SleepImpl::set_duration(double duration)
{
  duration_ = duration;
  return *this;
}

SleepImpl* SleepImpl::start()
{
  model_action_ = host_->get_cpu()->sleep(duration_);
  model_action_->set_activity(this);
  XBT_DEBUG("Create sleep synchronization %p", this);
  return this;
}

void SleepImpl::finish()
{
  if (model_action_->get_state() == resource::Action::State::FAILED) {
    if (host_ && not host_->is_on())
      set_state(State::SRC_HOST_FAILURE);
    else
      set_state(State::CANCELED);
  } else if (model_action_->get_state() == resource::Action::State::FINISHED) {
    set_state(State::DONE);
  }

  clean_action();
  XBT_DEBUG("SleepImpl::finish() in state %s", get_state_str());
  while (not simcalls_.empty()) {
    auto issuer = unregister_first_simcall();
    if (issuer == nullptr) /* don't answer exiting and dying actors */
      continue;

    if (issuer->is_suspended()) {
      XBT_DEBUG("Wait! This actor is suspended and can't wake up now.");
      issuer->suspended_ = false;
      issuer->suspend();
    } else {
      issuer->simcall_answer();
    }
  }
}
} // namespace simgrid::kernel::activity
