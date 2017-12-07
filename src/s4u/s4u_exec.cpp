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

void Exec::start()
{
  pimpl_ = simcall_execution_start(nullptr, flops_amount_, 1 / priority_, 0.);
  state_ = started;
}

void Exec::wait()
{
  simcall_execution_wait(pimpl_);
}

void Exec::wait(double timeout)
{
  THROW_UNIMPLEMENTED;
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

  return false;
}

ExecPtr Exec::setPriority(double priority)
{
  priority_ = priority;
  return this;
}

double Exec::getRemains()
{
  return simgrid::simix::kernelImmediate(
      [this]() { return boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->remains(); });
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
