/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MUTEX_HPP
#define SIMGRID_S4U_MUTEX_HPP

#include <utility>

#include <boost/intrusive_ptr.hpp>
#include <xbt/base.h>
#include "simgrid/simix.h"

namespace simgrid {
namespace s4u {

XBT_PUBLIC_CLASS Mutex {

public:
  Mutex() :
    mutex_(simcall_mutex_init()) {}
  Mutex(simgrid::simix::Mutex* mutex) : mutex_(SIMIX_mutex_ref(mutex)) {}
  ~Mutex()
  {
    SIMIX_mutex_unref(mutex_);
  }

  // Copy+move (with the copy-and-swap idiom):
  Mutex(Mutex const& mutex) : mutex_(SIMIX_mutex_ref(mutex.mutex_)) {}
  friend void swap(Mutex& first, Mutex& second)
  {
    using std::swap;
    swap(first.mutex_, second.mutex_);
  }
  Mutex& operator=(Mutex mutex)
  {
    swap(*this, mutex);
    return *this;
  }
  Mutex(Mutex&& mutex) : mutex_(nullptr)
  {
    swap(*this, mutex);
  }

  bool valid() const
  {
    return mutex_ != nullptr;
  }

public:
  void lock();
  void unlock();
  bool try_lock();

private:
  simgrid::simix::Mutex* mutex_;
};
}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_MUTEX_HPP */
