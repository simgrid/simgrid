/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SynchroRaw.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_interface.hpp"
#include <simgrid/s4u/Host.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_synchro, simix, "SIMIX Synchronization (mutex, semaphores and conditions)");

namespace simgrid {
namespace kernel {
namespace activity {

RawImpl& RawImpl::set_host(s4u::Host* host)
{
  host_ = host;
  return *this;
}
RawImpl& RawImpl::set_timeout(double timeout)
{
  timeout_ = timeout;
  return *this;
}

RawImpl* RawImpl::start()
{
  surf_action_ = host_->pimpl_cpu->sleep(timeout_);
  surf_action_->set_activity(this);
  return this;
}

void RawImpl::suspend()
{
  /* The suspension of raw synchros is delayed to when the process is rescheduled. */
}

void RawImpl::resume()
{
  /* I cannot resume raw synchros directly. This is delayed to when the process is rescheduled at
   * the end of the synchro. */
}

void RawImpl::cancel()
{
  /* I cannot cancel raw synchros directly. */
}

void RawImpl::post()
{
  if (surf_action_->get_state() == resource::Action::State::FAILED) {
    state_ = State::FAILED;
  } else if (surf_action_->get_state() == resource::Action::State::FINISHED) {
    state_ = State::SRC_TIMEOUT;
  }
  finish();
}

void RawImpl::finish()
{
  smx_simcall_t simcall = simcalls_.front();
  simcalls_.pop_front();

  if (state_ == State::FAILED) {
    XBT_DEBUG("RawImpl::finish(): host '%s' failed", simcall->issuer_->get_host()->get_cname());
    simcall->issuer_->context_->set_wannadie();
    simcall->issuer_->exception_ = std::make_exception_ptr(HostFailureException(XBT_THROW_POINT, "Host failed"));
  } else if (state_ != State::SRC_TIMEOUT) {
    xbt_die("Internal error in RawImpl::finish() unexpected synchro state %d", static_cast<int>(state_));
  }

  switch (simcall->call_) {
    case simix::Simcall::MUTEX_LOCK:
      simcall_mutex_lock__get__mutex(simcall)->remove_sleeping_actor(*simcall->issuer_);
      break;

    case simix::Simcall::COND_WAIT:
      simcall_cond_wait_timeout__get__cond(simcall)->remove_sleeping_actor(*simcall->issuer_);
      break;

    case simix::Simcall::COND_WAIT_TIMEOUT:
      simcall_cond_wait_timeout__get__cond(simcall)->remove_sleeping_actor(*simcall->issuer_);
      simcall_cond_wait_timeout__set__result(simcall, 1); // signal a timeout
      break;

    case simix::Simcall::SEM_ACQUIRE:
      simcall_sem_acquire_timeout__get__sem(simcall)->remove_sleeping_actor(*simcall->issuer_);
      break;

    case simix::Simcall::SEM_ACQUIRE_TIMEOUT:
      simcall_sem_acquire_timeout__get__sem(simcall)->remove_sleeping_actor(*simcall->issuer_);
      simcall_sem_acquire_timeout__set__result(simcall, 1); // signal a timeout
      break;

    default:
      THROW_IMPOSSIBLE;
  }
  simcall->issuer_->waiting_synchro_ = nullptr;
  simcall->issuer_->simcall_answer();
}

} // namespace activity
} // namespace kernel
} // namespace simgrid
