/* Copyright (c) 2018-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/ActivitySet.hpp>
#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Io.hpp>
#include <xbt/log.h>

#include "src/kernel/activity/IoImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_io, s4u_activity, "S4U asynchronous I/Os");

namespace simgrid::s4u {

Io::Io(kernel::activity::IoImplPtr pimpl)
{
  pimpl_ = pimpl;
}

IoPtr Io::init()
{
  auto pimpl = kernel::activity::IoImplPtr(new kernel::activity::IoImpl());
  return IoPtr(static_cast<Io*>(pimpl->get_iface()));
}

IoPtr Io::streamto_init(Host* from, const Disk* from_disk, Host* to, const Disk* to_disk)
{
  auto res = Io::init()->set_source(from, from_disk)->set_destination(to, to_disk);
  res->set_state(State::STARTING);
  return res;
}

IoPtr Io::streamto_async(Host* from, const Disk* from_disk, Host* to, const Disk* to_disk,
                         uint64_t simulated_size_in_bytes)
{
  return Io::init()->set_size(simulated_size_in_bytes)->set_source(from, from_disk)->set_destination(to, to_disk);
}

void Io::streamto(Host* from, const Disk* from_disk, Host* to, const Disk* to_disk, uint64_t simulated_size_in_bytes)
{
  streamto_async(from, from_disk, to, to_disk, simulated_size_in_bytes)->wait();
}

IoPtr Io::set_source(Host* from, const Disk* from_disk)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the source of an IO stream once it's started (state: %s)", to_c_str(state_));
  kernel::actor::simcall_object_access(pimpl_.get(), [this, from, from_disk] {
    boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_host(from);
    if (from_disk != nullptr)
      boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_disk(from_disk->get_impl());
  });
  // Setting 'source' may allow to start the activity, let's try
  if (state_ == State::STARTING && remains_ <= 0)
    XBT_DEBUG("This IO has a size of 0 byte. It cannot start yet");
  else
    start();

  return this;
}

IoPtr Io::set_destination(Host* to, const Disk* to_disk)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the source of an IO stream once it's started (state: %s)", to_c_str(state_));
  kernel::actor::simcall_object_access(pimpl_.get(), [this, to, to_disk] {
    boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_dst_host(to);
    if (to_disk != nullptr)
      boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_dst_disk(to_disk->get_impl());
  });
  // Setting 'destination' may allow to start the activity, let's try
  if (state_ == State::STARTING && remains_ <= 0)
    XBT_DEBUG("This IO has a size of 0 byte. It cannot start yet");
  else
    start();

  return this;
}

Io* Io::do_start()
{
  kernel::actor::simcall_answered(
      [this] { (*boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)).set_name(get_name()).start(); });

  if (suspended_)
    pimpl_->suspend();

  state_ = State::STARTED;
  fire_on_start();
  fire_on_this_start();
  return this;
}

ssize_t Io::deprecated_wait_any_for(const std::vector<IoPtr>& ios, double timeout) // XBT_ATTRIB_DEPRECATED_v339
{
  ActivitySet set;
  for (const auto& io : ios)
    set.push(io);

  auto* ret = set.wait_any_for(timeout).get();
  for (size_t i = 0; i < ios.size(); i++)
    if (ios[i].get() == ret)
      return i;

  return -1;
}

IoPtr Io::set_disk(const_sg_disk_t disk)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING, "Cannot set disk once the Io is started");

  kernel::actor::simcall_answered(
      [this, disk] { boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_disk(disk->get_impl()); });

  // Setting the disk may allow to start the activity, let's try
  if (state_ == State::STARTING)
    start();

 return this;
}

IoPtr Io::set_priority(double priority)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the priority of an io after its start");
  kernel::actor::simcall_object_access(pimpl_.get(), [this, priority] {
    boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_sharing_penalty(1. / priority);
  });
  return this;
}

IoPtr Io::set_size(sg_size_t size)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING, "Cannot set size once the Io is started");
  kernel::actor::simcall_object_access(
      pimpl_.get(), [this, size] { boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_size(size); });
  set_remaining(size);
  return this;
}

IoPtr Io::set_op_type(OpType type)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING, "Cannot set size once the Io is started");
  kernel::actor::simcall_object_access(
      pimpl_.get(), [this, type] { boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->set_type(type); });
  return this;
}

IoPtr Io::update_priority(double priority)
{
  kernel::actor::simcall_answered([this, priority] {
    boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->update_sharing_penalty(1. / priority);
  });
  return this;
}

/** @brief Returns the amount of flops that remain to be done */
double Io::get_remaining() const
{
  return kernel::actor::simcall_object_access(
      pimpl_.get(), [this]() { return boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_remaining(); });
}

sg_size_t Io::get_performed_ioops() const
{
  return boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_performed_ioops();
}

bool Io::is_assigned() const
{
  if (boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_host() == nullptr) {
    return boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_disk() != nullptr;
  } else {
    return boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_dst_host() != nullptr;
  }
}

} // namespace simgrid::s4u
