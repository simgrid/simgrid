/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_BOOST_CONTEXT_HPP
#define SIMGRID_SIMIX_BOOST_CONTEXT_HPP

#include <boost/version.hpp>
#if BOOST_VERSION < 106100
#include <boost/context/fcontext.hpp>
#else
#include <boost/context/detail/fcontext.hpp>
#endif

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

/** @brief Userspace context switching implementation based on Boost.Context */
class BoostContext : public Context {
public:
  BoostContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process);
  ~BoostContext() override;
  void stop() override;
  virtual void resume() = 0;

  static void swap(BoostContext* from, BoostContext* to);
  static BoostContext* getMaestro() { return maestro_context_; }
  static void setMaestro(BoostContext* maestro) { maestro_context_ = maestro; }

private:
  static BoostContext* maestro_context_;
  void* stack_ = nullptr;

#if BOOST_VERSION < 105600
  boost::context::fcontext_t* fc_ = nullptr;
  typedef intptr_t arg_type;
#elif BOOST_VERSION < 106100
  boost::context::fcontext_t fc_;
  typedef intptr_t arg_type;
#else
  boost::context::detail::fcontext_t fc_;
  typedef boost::context::detail::transfer_t arg_type;
#endif
#if HAVE_SANITIZE_ADDRESS_FIBER_SUPPORT
  const void* asan_stack_ = nullptr;
  size_t asan_stack_size_ = 0;
  bool asan_stop_         = false;
#endif

  static void wrapper(arg_type arg);
};

class SerialBoostContext : public BoostContext {
public:
  SerialBoostContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
      : BoostContext(std::move(code), cleanup_func, process)
  {
  }
  void suspend() override;
  void resume() override;

  static void run_all();

private:
  static unsigned long process_index_;
};

#if HAVE_THREAD_CONTEXTS
class ParallelBoostContext : public BoostContext {
public:
  ParallelBoostContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
      : BoostContext(std::move(code), cleanup_func, process)
  {
  }
  void suspend() override;
  void resume() override;

  static void initialize();
  static void finalize();
  static void run_all();

private:
  static simgrid::xbt::Parmap<smx_actor_t>* parmap_;
  static std::vector<ParallelBoostContext*> workers_context_;
  static std::atomic<uintptr_t> threads_working_;
  static xbt_os_thread_key_t worker_id_key_;
};
#endif

class BoostContextFactory : public ContextFactory {
public:
  BoostContextFactory();
  ~BoostContextFactory() override;
  Context* create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process) override;
  void run_all() override;

private:
  bool parallel_;
};
}}} // namespace

#endif
