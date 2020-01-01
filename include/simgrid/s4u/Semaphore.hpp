/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_SEMAPHORE_HPP
#define SIMGRID_S4U_SEMAPHORE_HPP

#include <simgrid/forward.h>
#include <simgrid/simix.h>

namespace simgrid {
namespace s4u {

/** @brief A classical semaphore, but blocking in the simulation world
 *  @ingroup s4u_api
 *
 * It is strictly impossible to use a real semaphore, such as
 * <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_init.html">sem_init</a>,
 * because it would block the whole simulation.
 * Instead, you should use the present class, that offers a very similar interface.
 *
 * As for any S4U object, Semaphores are using the @ref s4u_raii "RAII idiom" for memory management.
 * Use #create() to get a simgrid::s4u::SemaphorePtr to a newly created semaphore
 * and only manipulate simgrid::s4u::SemaphorePtr.
 *
 */
class XBT_PUBLIC Semaphore {
  smx_sem_t sem_;
  std::atomic_int_fast32_t refcount_{0};

  friend void intrusive_ptr_add_ref(Semaphore* sem);
  friend void intrusive_ptr_release(Semaphore* sem);

public:
  explicit Semaphore(unsigned int initial_capacity);
  ~Semaphore();

  // No copy:
  /** You cannot create a new semaphore by copying an existing one. Use SemaphorePtr instead */
  Semaphore(Semaphore const&) = delete;
  /** You cannot create a new semaphore by value assignment either. Use SemaphorePtr instead */
  Semaphore& operator=(Semaphore const&) = delete;

  /** Constructs a new semaphore */
  static SemaphorePtr create(unsigned int initial_capacity);

  void acquire();
  int acquire_timeout(double timeout);
  void release();
  int get_capacity();
  int would_block();
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_SEMAPHORE_HPP */
