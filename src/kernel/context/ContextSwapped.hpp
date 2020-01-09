/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_SWAPPED_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_SWAPPED_CONTEXT_HPP

#include "src/kernel/context/Context.hpp"

#include <memory>

namespace simgrid {
namespace kernel {
namespace context {
class SwappedContext;

class SwappedContextFactory : public ContextFactory {
  friend SwappedContext; // Reads whether we are in parallel mode
public:
  SwappedContextFactory()                             = default;
  SwappedContextFactory(const SwappedContextFactory&) = delete;
  SwappedContextFactory& operator=(const SwappedContextFactory&) = delete;
  void run_all() override;

private:
  /* For the sequential execution */
  unsigned long process_index_     = 0;       // next actor to execute
  SwappedContext* maestro_context_ = nullptr; // save maestro's context

  /* For the parallel execution, will be created lazily with the right parameters if needed (ie, in parallel) */
  std::unique_ptr<simgrid::xbt::Parmap<smx_actor_t>> parmap_{nullptr};
};

class SwappedContext : public Context {
public:
  SwappedContext(std::function<void()>&& code, smx_actor_t get_actor, SwappedContextFactory* factory);
  SwappedContext(const SwappedContext&) = delete;
  SwappedContext& operator=(const SwappedContext&) = delete;
  virtual ~SwappedContext();

  void suspend() override;
  virtual void resume();
  XBT_ATTRIB_NORETURN void stop() override;

  virtual void swap_into(SwappedContext* to) = 0; // Defined in Raw, Boost and UContext subclasses

  unsigned char* get_stack();

  static thread_local SwappedContext* worker_context_;

#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
  const void* asan_stack_   = nullptr;
  size_t asan_stack_size_   = 0;
  SwappedContext* asan_ctx_ = nullptr;
  bool asan_stop_           = false;
#endif

private:
  unsigned char* stack_ = nullptr;       /* the thread stack */
  SwappedContextFactory* const factory_; // for sequential and parallel run_all()

#if HAVE_VALGRIND_H
  unsigned int valgrind_stack_id_;
#endif
};

} // namespace context
} // namespace kernel
} // namespace simgrid
#endif
