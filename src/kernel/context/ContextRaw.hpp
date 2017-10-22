/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_BOOST_CONTEXT_HPP
#define SIMGRID_SIMIX_BOOST_CONTEXT_HPP

#include <functional>

#include "Context.hpp"
#include "src/internal_config.h"
#include "src/simix/smx_private.hpp"

namespace simgrid {
namespace kernel {
namespace context {

class RawContext;
class RawContextFactory;

/** @brief Fast context switching inspired from SystemV ucontexts.
  *
  * The main difference to the System V context is that Raw Contexts are much faster because they don't
  * preserve the signal mask when switching. This saves a system call (at least on Linux) on each context switch.
  */
class RawContext : public Context {
private:
  void* stack_ = nullptr;
  /** pointer to top the stack stack */
  void* stack_top_ = nullptr;

public:
  friend class RawContextFactory;
  RawContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process);
  ~RawContext() override;

  static void wrapper(void* arg);
  void stop() override;
  void suspend() override;
  void resume();

private:
  void suspend_serial();
  void suspend_parallel();
  void resume_serial();
  void resume_parallel();
};

class RawContextFactory : public ContextFactory {
public:
  RawContextFactory();
  ~RawContextFactory() override;
  RawContext* create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process) override;
  void run_all() override;

private:
  void run_all_serial();
  void run_all_parallel();
};
}}} // namespace

#endif
