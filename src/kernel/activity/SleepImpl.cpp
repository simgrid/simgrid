/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Host.hpp>

#include "src/kernel/activity/SleepImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/resource/CpuImpl.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_actor);

namespace simgrid {
namespace kernel {
namespace activity {

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
  surf_action_ = host_->get_cpu()->sleep(duration_);
  surf_action_->set_activity(this);
  XBT_DEBUG("Create sleep synchronization %p", this);
  return this;
}

void SleepImpl::post()
{
  if (surf_action_->get_state() == resource::Action::State::FAILED) {
    if (host_ && not host_->is_on())
      set_state(State::SRC_HOST_FAILURE);
    else
      set_state(State::CANCELED);
  } else if (surf_action_->get_state() == resource::Action::State::FINISHED) {
    set_state(State::DONE);
  }

  clean_action();
  /* Answer all simcalls associated with the synchro */
  finish();
}
void SleepImpl::set_exception(actor::ActorImpl* issuer)
{
  /* FIXME: Really, nothing bad can happen while we sleep? */
}
void SleepImpl::finish()
{
  XBT_DEBUG("SleepImpl::finish() in state %s", get_state_str());
  while (not simcalls_.empty()) {
    const s_smx_simcall* simcall = simcalls_.front();
    simcalls_.pop_front();

    simcall->issuer_->waiting_synchro_ = nullptr;
    if (simcall->issuer_->is_suspended()) {
      XBT_DEBUG("Wait! This actor is suspended and can't wake up now.");
      simcall->issuer_->suspended_ = false;
      simcall->issuer_->suspend();
    } else {
      simcall->issuer_->simcall_answer();
    }
  }
}
} // namespace activity
} // namespace kernel
} // namespace simgrid
