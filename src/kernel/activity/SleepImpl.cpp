/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SleepImpl.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/simix/popping_private.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);
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
  surf_action_ = host_->pimpl_cpu->sleep(duration_);
  surf_action_->set_activity(this);
  XBT_DEBUG("Create sleep synchronization %p", this);
  return this;
}

void SleepImpl::post()
{
  if (surf_action_->get_state() == resource::Action::State::FAILED) {
    if (host_ && not host_->is_on())
      state_ = SIMIX_SRC_HOST_FAILURE;
    else
      state_ = SIMIX_CANCELED;
  } else if (surf_action_->get_state() == resource::Action::State::FINISHED) {
    state_ = SIMIX_DONE;
  }
  /* Answer all simcalls associated with the synchro */
  finish();
}

void SleepImpl::finish()
{
  while (not simcalls_.empty()) {
    smx_simcall_t simcall = simcalls_.front();
    simcalls_.pop_front();

    simcall->issuer_->waiting_synchro = nullptr;
    if (simcall->issuer_->is_suspended()) {
      XBT_DEBUG("Wait! This process is suspended and can't wake up now.");
      simcall->issuer_->suspended_ = false;
      simcall->issuer_->suspend();
    } else {
      simcall->issuer_->simcall_answer();
    }
  }

  clean_action();
}
} // namespace activity
} // namespace kernel
} // namespace simgrid
