/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_SEMAPHOREIMPL_HPP
#define SIMGRID_KERNEL_ACTIVITY_SEMAPHOREIMPL_HPP

#include <atomic>
#include <boost/intrusive/list.hpp>

#include "simgrid/s4u/Semaphore.hpp"
#include "src/kernel/actor/ActorImpl.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC SemaphoreImpl {
  std::atomic_int_fast32_t refcount_{1};
  unsigned int value_;
  actor::SynchroList sleeping_; /* list of sleeping actors*/

public:
  explicit SemaphoreImpl(unsigned int value) : value_(value){};

  SemaphoreImpl(SemaphoreImpl const&) = delete;
  SemaphoreImpl& operator=(SemaphoreImpl const&) = delete;

  void acquire(actor::ActorImpl* issuer, double timeout);
  void release();
  bool would_block() const { return (value_ == 0); }
  void remove_sleeping_actor(actor::ActorImpl& actor) { xbt::intrusive_erase(sleeping_, actor); }

  unsigned int get_capacity() const { return value_; }
  bool is_used() const { return not sleeping_.empty(); }

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
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_ACTIVITY_SEMAPHOREIMPL_HPP */
