/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_UNIX_CONTEXT_HPP
#define SIMGRID_SIMIX_UNIX_CONTEXT_HPP

#include <ucontext.h> /* context relative declarations */

#include <cstdint>
#include <functional>
#include <vector>

#include <simgrid/simix.hpp>
#include <xbt/parmap.hpp>
#include <xbt/xbt_os_thread.h>

#include "Context.hpp"
#include "src/internal_config.h"
#include "src/simix/smx_private.hpp"

namespace simgrid {
namespace kernel {
namespace context {

class UContext;
class SerialUContext;
class ParallelUContext;
class UContextFactory;

class UContext : public Context {
private:
  ucontext_t uc_;         /* the ucontext that executes the code */
  char* stack_ = nullptr; /* the thread stack */
public:
  friend UContextFactory;
  UContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process);
  ~UContext() override;
  void stop() override;
  static void swap(UContext* from, UContext* to) { swapcontext(&from->uc_, &to->uc_); }
};

class SerialUContext : public UContext {
public:
  SerialUContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
      : UContext(std::move(code), cleanup_func, process)
  {
  }
  void suspend() override;
  void resume();
};

class ParallelUContext : public UContext {
public:
  ParallelUContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
      : UContext(std::move(code), cleanup_func, process)
  {
  }
  void suspend() override;
  void resume();
};

class UContextFactory : public ContextFactory {
public:
  friend UContext;
  friend SerialUContext;
  friend ParallelUContext;

  UContextFactory();
  ~UContextFactory() override;
  Context* create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process) override;
  void run_all() override;
};
}}} // namespace

#endif
