/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_exec, s4u_activity, "S4U asynchronous executions");

namespace simgrid {
namespace s4u {
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Exec::on_start;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Exec::on_completion;

Exec* Exec::start()
{
  pimpl_ = simcall_execution_start(name_, tracing_category_, flops_amount_, 1. / priority_, bound_, host_);
  state_ = State::STARTED;
  on_start(Actor::self());
  return this;
}

Exec* Exec::cancel()
{
  simgrid::simix::simcall([this] { dynamic_cast<kernel::activity::ExecImpl*>(pimpl_.get())->cancel(); });
  state_ = State::CANCELED;
  return this;
}

Exec* Exec::wait()
{
  if (state_ == State::INITED)
    start();
  simcall_execution_wait(pimpl_);
  state_ = State::FINISHED;
  on_completion(Actor::self());
  return this;
}

Exec* Exec::wait_for(double timeout)
{
  THROW_UNIMPLEMENTED;
  return this;
}

/** @brief Returns whether the state of the exec is finished */
bool Exec::test()
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED || state_ == State::FINISHED);

  if (state_ == State::FINISHED)
    return true;

  if (state_ == State::INITED)
    this->start();

  if (simcall_execution_test(pimpl_)) {
    state_ = State::FINISHED;
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
  xbt_assert(state_ == State::INITED, "Cannot change the priority of an exec after its start");
  priority_ = priority;
  return this;
}

/** @brief change the execution bound, ie the maximal amount of flops per second that it may consume, regardless of what
 * the host may deliver
 *
 * Currently, this cannot be changed once the exec started. */
ExecPtr Exec::set_bound(double bound)
{
  xbt_assert(state_ == State::INITED, "Cannot change the bound of an exec after its start");
  bound_ = bound;
  return this;
}

/** @brief Change the host on which this activity takes place.
 *
 * The activity cannot be terminated already (but it may be started). */
ExecPtr Exec::set_host(Host* host)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED,
             "Cannot change the host of an exec once it's done (state: %d)", (int)state_);
  if (state_ == State::STARTED)
    boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->migrate(host);
  host_ = host;
  return this;
}

ExecPtr Exec::set_name(std::string name)
{
  xbt_assert(state_ == State::INITED, "Cannot change the name of an exec after its start");
  name_ = name;
  return this;
}

ExecPtr Exec::set_tracing_category(std::string category)
{
  xbt_assert(state_ == State::INITED, "Cannot change the tracing category of an exec after its start");
  tracing_category_ = category;
  return this;
}

/** @brief Retrieve the host on which this activity takes place. */
Host* Exec::get_host()
{
  return host_;
}

/** @brief Returns the amount of flops that remain to be done */
double Exec::get_remaining()
{
  return simgrid::simix::simcall(
      [this]() { return boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->get_remaining(); });
}

/** @brief Returns the ratio of elements that are still to do
 *
 * The returned value is between 0 (completely done) and 1 (nothing done yet).
 */
double Exec::get_remaining_ratio()
{
  return simgrid::simix::simcall([this]() {
    return boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->get_remaining_ratio();
  });
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
