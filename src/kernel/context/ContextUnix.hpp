/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_UNIX_CONTEXT_HPP
#define SIMGRID_SIMIX_UNIX_CONTEXT_HPP

#include <ucontext.h> /* context relative declarations */

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include <simgrid/simix.hpp>
#include <xbt/parmap.hpp>
#include <xbt/xbt_os_thread.h>

#include "src/internal_config.h"
#include "src/kernel/context/ContextSwapped.hpp"

namespace simgrid {
namespace kernel {
namespace context {

class UContext : public SwappedContext {
public:
  UContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process,
           SwappedContextFactory* factory);

  void swap_into(SwappedContext* to) override;

private:
  ucontext_t uc_;         /* the ucontext that executes the code */

#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
  const void* asan_stack_ = nullptr;
  size_t asan_stack_size_ = 0;
  UContext* asan_ctx_     = nullptr;
  bool asan_stop_         = false;
#endif

  static void smx_ctx_sysv_wrapper(int, int);
  static void make_ctx(ucontext_t* ucp, void (*func)(int, int), UContext* arg);
};

class UContextFactory : public SwappedContextFactory {
public:
  UContextFactory() : SwappedContextFactory("UContextFactory") {}

  Context* create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process) override;
};
}}} // namespace

#endif
