/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_RAW_CONTEXT_HPP
#define SIMGRID_SIMIX_RAW_CONTEXT_HPP

#include <atomic>
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

/** @brief Fast context switching inspired from SystemV ucontexts.
  *
  * The main difference to the System V context is that Raw Contexts are much faster because they don't
  * preserve the signal mask when switching. This saves a system call (at least on Linux) on each context switch.
  */
class RawContext : public Context {
public:
  RawContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process);
  ~RawContext() override;
  void stop() override;
  virtual void resume() = 0;

  static void swap(RawContext* from, RawContext* to);
  static RawContext* getMaestro() { return maestro_context_; }
  static void setMaestro(RawContext* maestro) { maestro_context_ = maestro; }

private:
  static RawContext* maestro_context_;
  void* stack_ = nullptr;
  /** pointer to top the stack stack */
  void* stack_top_ = nullptr;

  static void wrapper(void* arg);
};

class SerialRawContext : public RawContext {
public:
  SerialRawContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
      : RawContext(std::move(code), cleanup_func, process)
  {
  }
  void suspend() override;
  void resume() override;

  static void run_all();

private:
  static unsigned long process_index_;
};

#if HAVE_THREAD_CONTEXTS
class ParallelRawContext : public RawContext {
public:
  ParallelRawContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
      : RawContext(std::move(code), cleanup_func, process)
  {
  }
  void suspend() override;
  void resume() override;

  static void initialize();
  static void finalize();
  static void run_all();

private:
  static simgrid::xbt::Parmap<smx_actor_t>* parmap_;
  static std::vector<ParallelRawContext*> workers_context_;
  static std::atomic<uintptr_t> threads_working_;
  static xbt_os_thread_key_t worker_id_key_;
};
#endif

class RawContextFactory : public ContextFactory {
public:
  RawContextFactory();
  ~RawContextFactory() override;
  Context* create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process) override;
  void run_all() override;

private:
  bool parallel_;
};
}}} // namespace

#endif
