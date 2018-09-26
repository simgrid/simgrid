/* Copyright (c) 2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Io.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "src/kernel/activity/IoImpl.hpp"
#include "src/simix/smx_io_private.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_io, s4u_activity, "S4U asynchronous IOs");

namespace simgrid {
namespace s4u {

Io* Io::start()
{
  Activity::set_remaining(size_);
  pimpl_ = simix::simcall([this] {
    return boost::static_pointer_cast<kernel::activity::IoImpl>(SIMIX_io_start(name_, size_, storage_, type_));
  });
  state_ = State::STARTED;
  return this;
}

Io* Io::cancel()
{
  simgrid::simix::simcall([this] { dynamic_cast<kernel::activity::IoImpl*>(pimpl_.get())->cancel(); });
  state_ = State::CANCELED;
  return this;
}

Io* Io::wait()
{
  simcall_io_wait(pimpl_);
  state_ = State::FINISHED;
  return this;
}

Io* Io::wait_for(double timeout)
{
  THROW_UNIMPLEMENTED;
  return this;
}

bool Io::test()
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED || state_ == State::FINISHED);

  if (state_ == State::FINISHED)
    return true;

  if (state_ == State::INITED)
    this->start();

  THROW_UNIMPLEMENTED;

  return false;
}

/** @brief Returns the amount of flops that remain to be done */
double Io::get_remaining()
{
  return simgrid::simix::simcall(
      [this]() { return boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_remaining(); });
}

sg_size_t Io::get_performed_ioops()
{
  return simgrid::simix::simcall(
      [this]() { return boost::static_pointer_cast<kernel::activity::IoImpl>(pimpl_)->get_performed_ioops(); });
}

void intrusive_ptr_release(simgrid::s4u::Io* i)
{
  if (i->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete i;
  }
}

void intrusive_ptr_add_ref(simgrid::s4u::Io* i)
{
  i->refcount_.fetch_add(1, std::memory_order_relaxed);
}
} // namespace s4u
} // namespace simgrid
