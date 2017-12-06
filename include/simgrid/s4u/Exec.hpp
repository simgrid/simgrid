/* Copyright (c) 2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_EXEC_HPP
#define SIMGRID_S4U_EXEC_HPP

#include "src/kernel/activity/ExecImpl.hpp"
#include <simgrid/forward.h>
#include <simgrid/s4u/forward.hpp>

namespace simgrid {
namespace s4u {

XBT_PUBLIC_CLASS Exec : public Activity
{
  Exec() : Activity() {}
public:
  friend void intrusive_ptr_release(simgrid::s4u::Exec * e)
  {
    if (e->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete e;
    }
  }

  friend void intrusive_ptr_add_ref(simgrid::s4u::Exec * e) { e->refcount_.fetch_add(1, std::memory_order_relaxed); }

  friend Actor; // Factory of Exec

  ~Exec() = default;

  void start() override
  {
    pimpl_ = simcall_execution_start(nullptr, flops_amount_, 1 / priority_, 0.);
    state_ = started;
  }
  void wait() override { this->wait(-1); }
  void wait(double timeout) override { simcall_execution_wait(pimpl_); }

  double getRemains()
  {
    return simgrid::simix::kernelImmediate(
        [this]() { return boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->remains(); });
  }

private:
  smx_actor_t runner_  = nullptr;
  double flops_amount_ = 0.0;
  double priority_     = 1.0;
  double bound_        = 0.0;

  std::atomic_int_fast32_t refcount_{0};
}; // class
}
}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_EXEC_HPP */
