/* Copyright (c) 2018-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Io.hpp>
#include <xbt/log.h>

#include "src/kernel/activity/IoImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"

namespace simgrid {
namespace s4u {
xbt::signal<void(Io const&)> Io::on_start;

Io::Io(kernel::activity::IoImplPtr pimpl)
{
  pimpl_ = pimpl;
}

IoPtr Io::init()
{
  auto pimpl = kernel::activity::IoImplPtr(new kernel::activity::IoImpl());
  return IoPtr(static_cast<Io*>(pimpl->get_iface()));
}

Io* Io::start()
{
  kernel::actor::simcall(
      [this] { (*boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)).set_name(get_name()).start(); });

  if (suspended_)
    pimpl_->suspend();

  state_ = State::STARTED;
  on_start(*this);
  return this;
}

ssize_t Io::wait_any_for(const std::vector<IoPtr>& ios, double timeout)
{
  std::vector<kernel::activity::IoImpl*> rios(ios.size());
  std::transform(begin(ios), end(ios), begin(rios),
                 [](const IoPtr& io) { return static_cast<kernel::activity::IoImpl*>(io->pimpl_.get()); });

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::IoWaitanySimcall observer{issuer, rios, timeout};
  ssize_t changed_pos = kernel::actor::simcall_blocking(
      [&observer] {
        kernel::activity::IoImpl::wait_any_for(observer.get_issuer(), observer.get_ios(), observer.get_timeout());
      },
      &observer);
  if (changed_pos != -1)
    ios.at(changed_pos)->complete(State::FINISHED);
  return changed_pos;
}

IoPtr Io::set_disk(const_sg_disk_t disk)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING, "Cannot set disk once the Io is started");

  kernel::actor::simcall(
      [this, disk] { boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_disk(disk->get_impl()); });

  // Setting the disk may allow to start the activity, let's try
  if (state_ == State::STARTING)
    vetoable_start();

 return this;
}

IoPtr Io::set_priority(double priority)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the priority of an io after its start");
  kernel::actor::simcall([this, priority] {
    boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_sharing_penalty(1. / priority);
  });
  return this;
}

IoPtr Io::set_size(sg_size_t size)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING, "Cannot set size once the Io is started");
  kernel::actor::simcall(
      [this, size] { boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_size(size); });
  Activity::set_remaining(size);
  return this;
}

IoPtr Io::set_op_type(OpType type)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING, "Cannot set size once the Io is started");
  kernel::actor::simcall(
      [this, type] { boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_type(type); });
  return this;
}

IoPtr Io::update_priority(double priority)
{
  kernel::actor::simcall([this, priority] {
    boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->update_sharing_penalty(1. / priority);
  });
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

bool Io::is_assigned() const
{
  return boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_disk() != nullptr;
}

} // namespace s4u
} // namespace simgrid
