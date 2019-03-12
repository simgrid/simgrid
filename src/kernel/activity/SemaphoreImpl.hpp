/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_SEMAPHOREIMPL_HPP_
#define SIMGRID_KERNEL_ACTIVITY_SEMAPHOREIMPL_HPP_

#include <atomic>
#include <boost/intrusive/list.hpp>

#include "simgrid/s4u/Semaphore.hpp"
#include "src/kernel/actor/ActorImpl.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC SemaphoreImpl {
public:
  explicit SemaphoreImpl(unsigned int value) : value_(value){};
  ~SemaphoreImpl() = default;

  SemaphoreImpl(SemaphoreImpl const&) = delete;
  SemaphoreImpl& operator=(SemaphoreImpl const&) = delete;

  void acquire(smx_actor_t issuer, double timeout);
  void release();
  bool would_block() { return (value_ == 0); }
  unsigned int get_capacity() { return value_; }

  friend void intrusive_ptr_add_ref(SemaphoreImpl* sem)
  {
    XBT_ATTRIB_UNUSED auto previous = sem->refcount_.fetch_add(1);
    xbt_assert(previous != 0);
  }
  friend void intrusive_ptr_release(SemaphoreImpl* sem)
  {
    if (sem->refcount_.fetch_sub(1) == 1)
      delete sem;
  }

  unsigned int value_;
  actor::SynchroList sleeping_; /* list of sleeping actors*/

private:
  std::atomic_int_fast32_t refcount_{1};
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_ACTIVITY_SEMAPHOREIMPL_HPP_ */
