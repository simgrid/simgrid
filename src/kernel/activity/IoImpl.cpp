/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>

#include "mc/mc.h"
#include "src/kernel/activity/IoImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/mc/mc_replay.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_io, simix, "Logging specific to SIMIX (io)");

namespace simgrid {
namespace kernel {
namespace activity {

IoImpl::IoImpl()
{
  piface_ = new s4u::Io(this);
  actor::ActorImpl* self = actor::ActorImpl::self();
  if (self) {
    set_actor(self);
    self->activities_.emplace_back(this);
  }
}

IoImpl& IoImpl::set_sharing_penalty(double sharing_penalty)
{
  sharing_penalty_ = sharing_penalty;
  return *this;
}

IoImpl& IoImpl::update_sharing_penalty(double sharing_penalty)
{
  sharing_penalty_ = sharing_penalty;
  surf_action_->set_sharing_penalty(sharing_penalty);
  return *this;
}

IoImpl& IoImpl::set_timeout(double timeout)
{
  const s4u::Host* host = get_disk()->get_host();
  timeout_detector_     = host->get_cpu()->sleep(timeout);
  timeout_detector_->set_activity(this);
  return *this;
}

IoImpl& IoImpl::set_type(s4u::Io::OpType type)
{
  type_ = type;
  return *this;
}

IoImpl& IoImpl::set_size(sg_size_t size)
{
  size_ = size;
  return *this;
}

IoImpl& IoImpl::set_disk(resource::DiskImpl* disk)
{
  disk_ = disk;
  return *this;
}

IoImpl* IoImpl::start()
{
  state_ = State::RUNNING;
  surf_action_ =
      disk_->get_host()->get_netpoint()->get_englobing_zone()->get_disk_model()->io_start(disk_, size_, type_);
  surf_action_->set_sharing_penalty(sharing_penalty_);
  surf_action_->set_activity(this);
  start_time_ = surf_action_->get_start_time();

  XBT_DEBUG("Create IO synchro %p %s", this, get_cname());

  return this;
}

void IoImpl::post()
{
  performed_ioops_ = surf_action_->get_cost();
  if (surf_action_->get_state() == resource::Action::State::FAILED) {
    if (disk_ && not disk_->is_on())
      state_ = State::FAILED;
    else
      state_ = State::CANCELED;
  } else if (timeout_detector_ && timeout_detector_->get_state() == resource::Action::State::FINISHED) {
    if (surf_action_->get_remains() > 0.0) {
      surf_action_->set_state(resource::Action::State::FAILED);
      state_ = State::TIMEOUT;
    } else {
      state_ = State::DONE;
    }
  } else {
    state_ = State::DONE;
  }

  clean_action();
  if (timeout_detector_) {
    timeout_detector_->unref();
    timeout_detector_ = nullptr;
  }

  /* Answer all simcalls associated with the synchro */
  finish();
}
void IoImpl::set_exception(actor::ActorImpl* issuer)
{
  switch (state_) {
    case State::FAILED:
      issuer->context_->set_wannadie();
      static_cast<s4u::Io*>(get_iface())->complete(s4u::Activity::State::FAILED);
      issuer->exception_ = std::make_exception_ptr(StorageFailureException(XBT_THROW_POINT, "Storage failed"));
      break;
    case State::CANCELED:
      issuer->exception_ = std::make_exception_ptr(CancelException(XBT_THROW_POINT, "I/O Canceled"));
      break;
    case State::TIMEOUT:
      issuer->exception_ = std::make_exception_ptr(TimeoutException(XBT_THROW_POINT, "Timeouted"));
      break;
    default:
      xbt_assert(state_ == State::DONE, "Internal error in IoImpl::finish(): unexpected synchro state %s",
                 to_c_str(state_));
  }
}

void IoImpl::finish()
{
  XBT_DEBUG("IoImpl::finish() in state %s", to_c_str(state_));
  while (not simcalls_.empty()) {
    smx_simcall_t simcall = simcalls_.front();
    simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
     * simcall */

    if (simcall->call_ == simix::Simcall::NONE) // FIXME: maybe a better way to handle this case
      continue;                                 // if process handling comm is killed
    if (auto* observer = dynamic_cast<kernel::actor::IoWaitanySimcall*>(simcall->observer_)) { // simcall is a wait_any?
      const auto& ios = observer->get_ios();

      for (auto* io : ios) {
        io->unregister_simcall(simcall);

        if (simcall->timeout_cb_) {
          simcall->timeout_cb_->remove();
          simcall->timeout_cb_ = nullptr;
        }
      }

      if (not MC_is_active() && not MC_record_replay_is_active()) {
        auto element = std::find(ios.begin(), ios.end(), this);
        int rank     = element != ios.end() ? static_cast<int>(std::distance(ios.begin(), element)) : -1;
        observer->set_result(rank);
      }
    }

    set_exception(simcall->issuer_);

    simcall->issuer_->waiting_synchro_ = nullptr;
    simcall->issuer_->simcall_answer();
  }
}

void IoImpl::wait_any_for(actor::ActorImpl* issuer, const std::vector<IoImpl*>& ios, double timeout)
{
  if (timeout < 0.0) {
    issuer->simcall_.timeout_cb_ = nullptr;
  } else {
    issuer->simcall_.timeout_cb_ = timer::Timer::set(s4u::Engine::get_clock() + timeout, [issuer, &ios]() {
      issuer->simcall_.timeout_cb_ = nullptr;
      for (auto* io : ios)
        io->unregister_simcall(&issuer->simcall_);
      // default result (-1) is set in actor::IoWaitanySimcall
      issuer->simcall_answer();
    });
  }

  for (auto* io : ios) {
    /* associate this simcall to the the synchro */
    io->simcalls_.push_back(&issuer->simcall_);

    /* see if the synchro is already finished */
    if (io->state_ != State::WAITING && io->state_ != State::RUNNING) {
      io->finish();
      break;
    }
  }
}

} // namespace activity
} // namespace kernel
} // namespace simgrid
