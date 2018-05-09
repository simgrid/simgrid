/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "xbt/log.h"

#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "src/kernel/activity/ExecImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_exec, s4u_activity, "S4U asynchronous executions");

namespace simgrid {
namespace s4u {

Activity* Exec::start()
{
  pimpl_ = simcall_execution_start(nullptr, flops_amount_, 1. / priority_, 0., host_);
  boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->setBound(bound_);
  state_ = State::started;
  return this;
}

Activity* Exec::wait()
{
  simcall_execution_wait(pimpl_);
  state_ = State::finished;
  return this;
}

Activity* Exec::wait(double timeout)
{
  THROW_UNIMPLEMENTED;
  return this;
}

/** @brief Returns whether the state of the exec is finished */
bool Exec::test()
{
  xbt_assert(state_ == State::inited || state_ == State::started || state_ == State::finished);

  if (state_ == State::finished)
    return true;

  if (state_ == State::inited)
    this->start();

  if (simcall_execution_test(pimpl_)) {
    state_ = State::finished;
    return true;
  }

  return false;
}

/** @brief  Change the execution priority, don't you think?
 *
 * An execution with twice the priority will get twice the amount of flops when the resource is shared.
 * The default priority is 1.
 *
 * Currently, this cannot be changed once the exec started. */
ExecPtr Exec::set_priority(double priority)
{
  xbt_assert(state_ == State::inited, "Cannot change the priority of an exec after its start");
  priority_ = priority;
  return this;
}

/** @brief change the execution bound, ie the maximal amount of flops per second that it may consume, regardless of what
 * the host may deliver
 *
 * Currently, this cannot be changed once the exec started. */
ExecPtr Exec::set_bound(double bound)
{
  xbt_assert(state_ == State::inited, "Cannot change the bound of an exec after its start");
  bound_ = bound;
  return this;
}

/** @brief Change the host on which this activity takes place.
 *
 * The activity cannot be terminated already (but it may be started). */
ExecPtr Exec::set_host(Host* host)
{
  xbt_assert(state_ == State::inited || state_ == State::started,
             "Cannot change the host of an exec once it's done (state: %d)", (int)state_);
  if (state_ == State::started)
    boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->migrate(host);
  host_ = host;
  return this;
}

/** @brief Retrieve the host on which this activity takes place. */
Host* Exec::get_host()
{
  return host_;
}

double Exec::get_remaining()
{
  return simgrid::simix::kernelImmediate(
      [this]() { return boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->remains(); });
}
double Exec::getRemainingRatio()
{
  return simgrid::simix::kernelImmediate(
      [this]() { return boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->remainingRatio(); });
}

void intrusive_ptr_release(simgrid::s4u::Exec* e)
{
  if (e->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete e;
  }
}

void intrusive_ptr_add_ref(simgrid::s4u::Exec* e)
{
  e->refcount_.fetch_add(1, std::memory_order_relaxed);
}
} // namespace s4u
} // namespace simgrid
