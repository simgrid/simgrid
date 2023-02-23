/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>

#include "src/kernel/activity/IoImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/kernel/resource/HostImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_io, kernel, "Kernel io-related synchronization");

namespace simgrid::kernel::activity {

IoImpl::IoImpl()
{
  piface_ = new s4u::Io(this);
  actor::ActorImpl* self = actor::ActorImpl::self();
  if (self) {
    set_actor(self);
    self->activities_.insert(this);
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
  model_action_->set_sharing_penalty(sharing_penalty);
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

IoImpl& IoImpl::set_host(s4u::Host* host)
{
  host_ = host;
  return *this;
}

IoImpl& IoImpl::set_dst_disk(resource::DiskImpl* disk)
{
  dst_disk_ = disk;
  return *this;
}

IoImpl& IoImpl::set_dst_host(s4u::Host* host)
{
  dst_host_ = host;
  return *this;
}

IoImpl* IoImpl::start()
{
  set_state(State::RUNNING);
  if (dst_host_ == nullptr) {
    XBT_DEBUG("Starting regular I/O");
    model_action_ = disk_->io_start(size_, type_);
    model_action_->set_sharing_penalty(sharing_penalty_);
  } else {
    XBT_DEBUG("Starting streaming I/O");
    auto host_model = dst_host_->get_netpoint()->get_englobing_zone()->get_host_model();
    model_action_   = host_model->io_stream(host_, disk_, dst_host_, dst_disk_, size_);
  }

  model_action_->set_activity(this);
  set_start_time(model_action_->get_start_time());

  XBT_DEBUG("Create IO synchro %p %s", this, get_cname());

  return this;
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
    case State::SRC_HOST_FAILURE:
    case State::DST_HOST_FAILURE:
       issuer->set_wannadie();
       static_cast<s4u::Io*>(get_iface())->complete(s4u::Activity::State::FAILED);
       issuer->exception_ = std::make_exception_ptr(StorageFailureException(XBT_THROW_POINT, "Host failed"));
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
  if (model_action_ != nullptr) {
    performed_ioops_ = model_action_->get_cost();
    if (model_action_->get_state() == resource::Action::State::FAILED) {
      if (host_ && dst_host_) { // this is an I/O stream
        if (not host_->is_on())
          set_state(State::SRC_HOST_FAILURE);
        else if (not dst_host_->is_on())
          set_state(State::DST_HOST_FAILURE);
      } else if ((disk_ && not disk_->is_on()) || (dst_disk_ && not dst_disk_->is_on()))
        set_state(State::FAILED);
      else
        set_state(State::CANCELED);
    } else {
      set_state(State::DONE);
    }

    clean_action();
  }

  while (not simcalls_.empty()) {
    actor::Simcall* simcall = simcalls_.front();
    simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
     * simcall */

    if (simcall->call_ == actor::Simcall::Type::NONE) // FIXME: maybe a better way to handle this case
      continue;                                       // if process handling comm is killed

    handle_activity_waitany(simcall);

    if (not simcall->issuer_->get_host()->is_on()) {
      simcall->issuer_->set_wannadie();
    } else {
      // Do not answer to dying actors
      if (not simcall->issuer_->wannadie()) {
        set_exception(simcall->issuer_);
        simcall->issuer_->waiting_synchro_ = nullptr;
        simcall->issuer_->simcall_answer();
      }
    }
  }
}

} // namespace simgrid::kernel::activity
