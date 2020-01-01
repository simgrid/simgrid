/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MUTEX_HPP
#define SIMGRID_S4U_MUTEX_HPP

#include <simgrid/forward.h>
#include <xbt/asserts.h>

namespace simgrid {
namespace s4u {

/** @brief A classical mutex, but blocking in the simulation world
 *  @ingroup s4u_api
 *
 * It is strictly impossible to use a real mutex, such as
 * <a href="http://en.cppreference.com/w/cpp/thread/mutex">std::mutex</a>
 * or <a href="http://pubs.opengroup.org/onlinepubs/007908775/xsh/pthread_mutex_lock.html">pthread_mutex_t</a>,
 * because it would block the whole simulation.
 * Instead, you should use the present class, that is a drop-in replacement of
 * <a href="http://en.cppreference.com/w/cpp/thread/mutex">std::mutex</a>.
 *
 * As for any S4U object, Mutexes are using the @ref s4u_raii "RAII idiom" for memory management.
 * Use create() to get a simgrid::s4u::MutexPtr to a newly created mutex and only manipulate simgrid::s4u::MutexPtr.
 *
 */
class XBT_PUBLIC Mutex {
  friend ConditionVariable;
  friend kernel::activity::MutexImpl;

  kernel::activity::MutexImpl* const pimpl_;
  /* refcounting */
  friend XBT_PUBLIC void intrusive_ptr_add_ref(const Mutex* mutex);
  friend XBT_PUBLIC void intrusive_ptr_release(const Mutex* mutex);

public:
  explicit Mutex(kernel::activity::MutexImpl* mutex) : pimpl_(mutex) {}
  ~Mutex();
  // No copy:
  /** You cannot create a new mutex by copying an existing one. Use MutexPtr instead */
  Mutex(Mutex const&) = delete;
  /** You cannot create a new mutex by value assignment either. Use MutexPtr instead */
  Mutex& operator=(Mutex const&) = delete;

  /** Constructs a new mutex */
  static MutexPtr create();
  void lock();
  void unlock();
  bool try_lock();
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_MUTEX_HPP */
