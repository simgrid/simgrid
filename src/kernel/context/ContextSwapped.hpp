/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_SWAPPED_CONTEXT_HPP
#define SIMGRID_SIMIX_SWAPPED_CONTEXT_HPP

#include "src/kernel/context/Context.hpp"

namespace simgrid {
namespace kernel {
namespace context {

class SwappedContext : public Context {
public:
  SwappedContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process);
  virtual ~SwappedContext();

  virtual void suspend();
  virtual void resume();

  static void run_all();

  virtual void swap_into(SwappedContext* to) = 0;

  static SwappedContext* get_maestro() { return maestro_context_; }
  static void set_maestro(SwappedContext* maestro) { maestro_context_ = maestro; }

protected:
  void* stack_ = nullptr; /* the thread stack */

private:
  static unsigned long process_index_;
  static SwappedContext* maestro_context_;
};

} // namespace context
} // namespace kernel
} // namespace simgrid
#endif
