/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

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
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/mc/mc_replay.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_io, kernel, "Kernel io-related synchronization");

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
  set_state(State::RUNNING);
  surf_action_ =
      disk_->get_host()->get_netpoint()->get_englobing_zone()->get_disk_model()->io_start(disk_, size_, type_);
  surf_action_->set_sharing_penalty(sharing_penalty_);
  surf_action_->set_activity(this);
  set_start_time(surf_action_->get_start_time());

  XBT_DEBUG("Create IO synchro %p %s", this, get_cname());

  return this;
}

void IoImpl::post()
{
  performed_ioops_ = surf_action_->get_cost();
  if (surf_action_->get_state() == resource::Action::State::FAILED) {
    if (disk_ && not disk_->is_on())
      set_state(State::FAILED);
    else
      set_state(State::CANCELED);
  } else if (timeout_detector_ && timeout_detector_->get_state() == resource::Action::State::FINISHED) {
    if (surf_action_->get_remains() > 0.0) {
      surf_action_->set_state(resource::Action::State::FAILED);
      set_state(State::TIMEOUT);
    } else {
      set_state(State::DONE);
    }
  } else {
    set_state(State::DONE);
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
  switch (get_state()) {
    case State::FAILED:
      issuer->set_wannadie();
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
      xbt_assert(get_state() == State::DONE, "Internal error in IoImpl::finish(): unexpected synchro state %s",
                 get_state_str());
  }
}

void IoImpl::finish()
{
  XBT_DEBUG("IoImpl::finish() in state %s", get_state_str());
  while (not simcalls_.empty()) {
    actor::Simcall* simcall = simcalls_.front();
    simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
     * simcall */

    if (simcall->call_ == actor::Simcall::Type::NONE) // FIXME: maybe a better way to handle this case
      continue;                                       // if process handling comm is killed

    handle_activity_waitany(simcall);

    set_exception(simcall->issuer_);

    simcall->issuer_->waiting_synchro_ = nullptr;
    simcall->issuer_->simcall_answer();
  }
}

} // namespace activity
} // namespace kernel
} // namespace simgrid
