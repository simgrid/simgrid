/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Host.hpp>

#include "src/kernel/activity/TimeoutDetector.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_timeout, kernel, "Kernel internal activity to detect timeouts");

namespace simgrid::kernel::activity {

TimeoutDetector& TimeoutDetector::set_host(s4u::Host* host)
{
  host_ = host;
  return *this;
}
TimeoutDetector& TimeoutDetector::set_timeout(double timeout)
{
  xbt_assert(timeout >= 0, "Timeout cannot be %f", timeout);
  timeout_ = timeout;
  return *this;
}

TimeoutDetector* TimeoutDetector::start()
{
  model_action_ = host_->get_cpu()->sleep(timeout_);
  model_action_->set_activity(this);
  return this;
}

void TimeoutDetector::suspend()
{
  /* The suspension of timeout detectors is delayed to when the actor is rescheduled. */
}

void TimeoutDetector::resume()
{
  /* I cannot resume timeout detectors directly. This is delayed to when the actor is rescheduled at
   * the end of the synchro. */
}

void TimeoutDetector::cancel()
{
  /* I cannot cancel timeout detectors directly. */
}

void TimeoutDetector::finish()
{
  XBT_DEBUG("TimeoutDetector::finish() in state %s", get_state_str());
  xbt_assert(simcalls_.size() == 1, "Unexpected number of simcalls waiting: %zu", simcalls_.size());
  actor::Simcall* simcall = simcalls_.front();
  simcalls_.pop_front();

  xbt_assert(model_action_->get_state() == resource::Action::State::FINISHED);
  set_state(State::SRC_TIMEOUT);
  clean_action();

  finish_callback_();
  simcall->issuer_->waiting_synchro_ = nullptr;
  simcall->issuer_->simcall_answer();
}

} // namespace simgrid::kernel::activity
