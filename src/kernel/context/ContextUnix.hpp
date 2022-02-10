/* Copyright (c) 2009-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_UNIX_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_UNIX_CONTEXT_HPP

#include <ucontext.h> /* context relative declarations */

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include <simgrid/simix.hpp>
#include <xbt/parmap.hpp>

#include "src/internal_config.h"
#include "src/kernel/context/ContextSwapped.hpp"

namespace simgrid {
namespace kernel {
namespace context {

class UContext : public SwappedContext {
public:
  UContext(std::function<void()>&& code, actor::ActorImpl* actor, SwappedContextFactory* factory);

private:
  ucontext_t uc_{}; /* the ucontext that executes the code */

  void swap_into_for_real(SwappedContext* to) override;
};

class UContextFactory : public SwappedContextFactory {
public:
  UContext* create_context(std::function<void()>&& code, actor::ActorImpl* actor) override;
};
} // namespace context
} // namespace kernel
} // namespace simgrid

#endif
