/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_SWAPPED_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_SWAPPED_CONTEXT_HPP

#include "src/internal_config.h" // HAVE_SANITIZER_*
#include "src/kernel/context/Context.hpp"

#include <memory>

namespace simgrid {
namespace kernel {
namespace context {
class SwappedContext;
} // namespace context
} // namespace kernel
} // namespace simgrid

/* Use extern "C" to make sure that this symbol is easy to recognize by name, even on exotic platforms */
extern "C" XBT_ATTRIB_NORETURN void smx_ctx_wrapper(simgrid::kernel::context::SwappedContext* context);

namespace simgrid {
namespace kernel {
namespace context {

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
  std::unique_ptr<simgrid::xbt::Parmap<actor::ActorImpl*>> parmap_{nullptr};
};

class SwappedContext : public Context {
  friend void ::smx_ctx_wrapper(simgrid::kernel::context::SwappedContext*);

public:
  SwappedContext(std::function<void()>&& code, actor::ActorImpl* get_actor, SwappedContextFactory* factory);
  SwappedContext(const SwappedContext&) = delete;
  SwappedContext& operator=(const SwappedContext&) = delete;
  ~SwappedContext() override;

  void suspend() override;
  virtual void resume();
  XBT_ATTRIB_NORETURN void stop() override;

  void swap_into(SwappedContext* to);

protected:
  unsigned char* get_stack() const { return stack_; }
  unsigned char* get_stack_bottom() const; // Depending on the stack direction, its bottom (that Boost::make_fcontext
                                           // needs) may be the lower or higher end

  // With ASan, after a context switch, check that the originating context is the expected one (see BoostContext)
  void verify_previous_context(const SwappedContext* context) const;

private:
  static thread_local SwappedContext* worker_context_;

  unsigned char* stack_ = nullptr; // the thread stack
  SwappedContextFactory& factory_; // for sequential and parallel run_all()

#if HAVE_VALGRIND_H
  unsigned int valgrind_stack_id_;
#endif
#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
  const void* asan_stack_   = nullptr;
  size_t asan_stack_size_   = 0;
  SwappedContext* asan_ctx_ = nullptr;
  bool asan_stop_           = false;
#endif
#if HAVE_SANITIZER_THREAD_FIBER_SUPPORT
  void* tsan_fiber_;
#endif

  virtual void swap_into_for_real(SwappedContext* to) = 0; // Defined in Raw, Boost and UContext subclasses
};

inline void SwappedContext::verify_previous_context(XBT_ATTRIB_UNUSED const SwappedContext* context) const
{
#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
  xbt_assert(this->asan_ctx_ == context);
#endif
}

} // namespace context
} // namespace kernel
} // namespace simgrid
#endif
