/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MUTEX_HPP
#define SIMGRID_S4U_MUTEX_HPP

#include <simgrid/forward.h>
#include <xbt/asserts.h>

namespace simgrid {
namespace s4u {

/** @brief A classical mutex, but blocking in the simulation world.
 *
 * @rst
 * It is strictly impossible to use a real mutex, such as
 * `std::mutex <http://en.cppreference.com/w/cpp/thread/mutex>`_
 * or `pthread_mutex_t <http://pubs.opengroup.org/onlinepubs/007908775/xsh/pthread_mutex_lock.html>`_,
 * because it would block the whole simulation.
 * Instead, you should use the present class, that is a drop-in replacement of these mechanisms.
 *
 * An example is available in Section :ref:`s4u_ex_IPC`.
 *
 * As for any S4U object, you can use the :ref:`RAII idiom <s4u_raii>` for memory management of Mutexes.
 * Use :cpp:func:`create() <simgrid::s4u::Mutex::create()>` to get a :cpp:type:`simgrid::s4u::MutexPtr` to a newly
 * created mutex, and only manipulate :cpp:type:`simgrid::s4u::MutexPtr`.
 * @endrst
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
#ifndef DOXYGEN
  Mutex(Mutex const&) = delete;            // No copy constructor; Use MutexPtr instead
  Mutex& operator=(Mutex const&) = delete; // No direct assignment either. Use MutexPtr instead
#endif

  /** Constructs a new mutex */
  static MutexPtr create();
  void lock();
  void unlock();
  bool try_lock();
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_MUTEX_HPP */
