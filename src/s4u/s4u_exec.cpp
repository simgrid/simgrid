/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

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
  pimpl_ = simcall_execution_start(nullptr, flops_amount_, 1 / priority_, 0., host_);
  state_ = started;
  return this;
}

Activity* Exec::wait()
{
  simcall_execution_wait(pimpl_);
  return this;
}

Activity* Exec::wait(double timeout)
{
  THROW_UNIMPLEMENTED;
  return this;
}

bool Exec::test()
{
  xbt_assert(state_ == inited || state_ == started || state_ == finished);

  if (state_ == finished) {
    return true;
  }

  if (state_ == inited) {
    this->start();
  }

  if (simcall_execution_test(pimpl_)) {
    state_ = finished;
    return true;
  }

  return false;
}

ExecPtr Exec::setPriority(double priority)
{
  xbt_assert(state_ == inited, "Cannot change the priority of an exec after its start");
  priority_ = priority;
  return this;
}
ExecPtr Exec::setHost(Host* host)
{
  xbt_assert(state_ == inited, "Cannot change the host of an exec after its start");
  host_ = host;
  return this;
}

double Exec::getRemains()
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
}
}
