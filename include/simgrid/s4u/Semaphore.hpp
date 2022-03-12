/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_SEMAPHORE_HPP
#define SIMGRID_S4U_SEMAPHORE_HPP

#include <simgrid/forward.h>

namespace simgrid {
namespace s4u {

/** @brief A classical semaphore, but blocking in the simulation world
 *
 * @beginrst
 * It is strictly impossible to use a real semaphore, such as
 * `sem_init <http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_init.html>`_,
 * because it would block the whole simulation.
 * Instead, you should use the present class, that offers a very similar interface.
 *
 * An example is available in Section :ref:`s4u_ex_IPC`.
 *
 * As for any S4U object, you can use the :ref:`RAII idiom <s4u_raii>` for memory management of semaphores.
 * Use :cpp:func:`create() <simgrid::s4u::Mutex::create()>` to get a :cpp:type:`simgrid::s4u::SemaphorePtr` to a newly
 * created semaphore, that will get automatically freed when the variable goes out of scope.
 * @endrst
 *
 */
class XBT_PUBLIC Semaphore {
#ifndef DOXYGEN
  friend kernel::activity::SemaphoreImpl;
  friend void kernel::activity::intrusive_ptr_release(kernel::activity::SemaphoreImpl* sem);
#endif

  kernel::activity::SemaphoreImpl* const pimpl_;

  friend void intrusive_ptr_add_ref(const Semaphore* sem);
  friend void intrusive_ptr_release(const Semaphore* sem);

  explicit Semaphore(kernel::activity::SemaphoreImpl* sem) : pimpl_(sem) {}
  ~Semaphore() = default;
#ifndef DOXYGEN
  Semaphore(Semaphore const&) = delete;            // No copy constructor. Use SemaphorePtr instead
  Semaphore& operator=(Semaphore const&) = delete; // No direct assignment either. Use SemaphorePtr instead
#endif

public:
  /** Constructs a new semaphore */
  static SemaphorePtr create(unsigned int initial_capacity);

  void acquire();
  /** Returns true if there was a timeout */
  bool acquire_timeout(double timeout);
  void release();
  int get_capacity() const;
  bool would_block() const;
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_SEMAPHORE_HPP */
