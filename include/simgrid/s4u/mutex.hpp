/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MUTEX_HPP
#define SIMGRID_S4U_MUTEX_HPP

#include <mutex>
#include <utility>

#include <boost/intrusive_ptr.hpp>

#include <xbt/base.h>
#include "simgrid/simix.h"

namespace simgrid {
namespace s4u {

class ConditionVariable;

XBT_PUBLIC_CLASS Mutex {
friend ConditionVariable;
private:
  friend simgrid::simix::Mutex;
  simgrid::simix::Mutex* mutex_;
  Mutex(simgrid::simix::Mutex* mutex) : mutex_(mutex) {}
public:

  friend void intrusive_ptr_add_ref(Mutex* mutex)
  {
    xbt_assert(mutex);
    SIMIX_mutex_ref(mutex->mutex_);
  }
  friend void intrusive_ptr_release(Mutex* mutex)
  {
    xbt_assert(mutex);
    SIMIX_mutex_unref(mutex->mutex_);
  }
  using Ptr = boost::intrusive_ptr<Mutex>;

  // No copy:
  Mutex(Mutex const&) = delete;
  Mutex& operatori(Mutex const&) = delete;

  static Ptr createMutex();

public:
  void lock();
  void unlock();
  bool try_lock();
};

using MutexPtr = Mutex::Ptr;

}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_MUTEX_HPP */
