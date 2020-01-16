/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_RAW_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_RAW_CONTEXT_HPP

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include <xbt/parmap.hpp>

#include "src/kernel/context/ContextSwapped.hpp"

namespace simgrid {
namespace kernel {
namespace context {

/** @brief Fast context switching inspired from SystemV ucontexts.
  *
  * The main difference to the System V context is that Raw Contexts are much faster because they don't
  * preserve the signal mask when switching. This saves a system call (at least on Linux) on each context switch.
  */
class RawContext : public SwappedContext {
public:
  RawContext(std::function<void()>&& code, actor::ActorImpl* actor, SwappedContextFactory* factory);

private:
  /** pointer to top the stack stack */
  void* stack_top_ = nullptr;

  void swap_into_for_real(SwappedContext* to) override;
};

class RawContextFactory : public SwappedContextFactory {
public:
  RawContext* create_context(std::function<void()>&& code, actor::ActorImpl* actor) override;
};
} // namespace context
} // namespace kernel
} // namespace simgrid

#endif
