/* Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Disk.hpp"
#include "simgrid/s4u/Io.hpp"
#include "src/kernel/activity/IoImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "xbt/log.h"

namespace simgrid {
namespace s4u {
xbt::signal<void(Io const&)> Io::on_start;
xbt::signal<void(Io const&)> Io::on_completion;

Io::Io()
{
  pimpl_ = kernel::activity::IoImplPtr(new kernel::activity::IoImpl());
}

IoPtr Io::init()
{
 return IoPtr(new Io());
}

Io* Io::start()
{
  kernel::actor::simcall([this] {
    (*boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_))
                  .set_name(get_name())
                  .set_disk(disk_->get_impl())
                  .set_size(size_)
                  .set_type(type_)
                  .start();
  });

  if (suspended_)
    pimpl_->suspend();

  state_ = State::STARTED;
  on_start(*this);
  return this;
}

Io* Io::cancel()
{
  simgrid::kernel::actor::simcall([this] { boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->cancel(); });
  state_ = State::CANCELED;
  on_completion(*this);
  return this;
}

Io* Io::wait()
{
  return this->wait_for(-1);
}

Io* Io::wait_for(double timeout)
{
  if (state_ == State::INITED)
    vetoable_start();

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::simcall_blocking<void>([this, issuer, timeout] { this->get_impl()->wait_for(issuer, timeout); });
  state_ = State::FINISHED;
  this->release_dependencies();

  on_completion(*this);
  return this;
}

IoPtr Io::set_disk(sg_disk_t disk)
{
  disk_ = disk;

  // Setting the disk may allow to start the activity, let's try
  if (state_ == State::STARTING)
    vetoable_start();

 return this;
}

IoPtr Io::set_size(sg_size_t size)
{
  size_ = size;
  Activity::set_remaining(size_);
  return this;
}

IoPtr Io::set_op_type(OpType type)
{
  type_ = type;
  return this;
}

/** @brief Returns the amount of flops that remain to be done */
double Io::get_remaining() const
{
  return kernel::actor::simcall(
      [this]() { return boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_remaining(); });
}

sg_size_t Io::get_performed_ioops() const
{
  return boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_performed_ioops();
}

} // namespace s4u
} // namespace simgrid
