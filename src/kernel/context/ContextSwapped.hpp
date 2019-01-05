/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_SWAPPED_CONTEXT_HPP
#define SIMGRID_SIMIX_SWAPPED_CONTEXT_HPP

#include "src/kernel/context/Context.hpp"

#include <vector>

namespace simgrid {
namespace kernel {
namespace context {

class SwappedContext : public Context {
public:
  SwappedContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process);
  virtual ~SwappedContext();

  static void initialize(); // Initialize the module, using the options
  static void finalize();   // Finalize the module

  virtual void suspend();
  virtual void resume();

  static void run_all();

  virtual void swap_into(SwappedContext* to) = 0; // Defined in subclasses

  static SwappedContext* get_maestro() { return maestro_context_; }
  static void set_maestro(SwappedContext* maestro) { maestro_context_ = maestro; }

protected:
  void* stack_ = nullptr; /* the thread stack */

  /* For the parallel execution */
  static simgrid::xbt::Parmap<smx_actor_t>* parmap_;
  static std::vector<SwappedContext*> workers_context_;
  static std::atomic<uintptr_t> threads_working_;
  static thread_local uintptr_t worker_id_;

private:
  static unsigned long process_index_;
  static SwappedContext* maestro_context_;
};

} // namespace context
} // namespace kernel
} // namespace simgrid
#endif
