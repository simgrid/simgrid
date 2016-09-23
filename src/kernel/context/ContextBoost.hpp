/* Copyright (c) 2015. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_BOOST_CONTEXT_HPP
#define SIMGRID_SIMIX_BOOST_CONTEXT_HPP

#include <functional>
#include <vector>

#include <xbt/parmap.h>

#include <simgrid/simix.hpp>


namespace simgrid {
namespace kernel {
namespace context {

class BoostContext;
class BoostSerialContext;
class BoostParallelContext;
class BoostContextFactory;

/** @brief Userspace context switching implementation based on Boost.Context */
class BoostContext : public Context {
protected: // static
  static bool parallel_;
  static xbt_parmap_t parmap_;
  static std::vector<BoostContext*> workers_context_;
  static uintptr_t threads_working_;
  static xbt_os_thread_key_t worker_id_key_;
  static unsigned long process_index_;
  static BoostContext* maestro_context_;
protected:
#if HAVE_BOOST_CONTEXTS == 1
  boost::context::fcontext_t* fc_ = nullptr;
#else
  boost::context::fcontext_t fc_;
#endif
  void* stack_ = nullptr;
public:
  friend BoostContextFactory;
  BoostContext(std::function<void()> code,
          void_pfn_smxprocess_t cleanup_func,
          smx_actor_t process);
  ~BoostContext() override;
  virtual void resume();
private:
  static void wrapper(int first, ...);
};

class BoostContextFactory : public ContextFactory {
public:
  friend BoostContext;
  friend BoostSerialContext;
  friend BoostParallelContext;

  BoostContextFactory();
  ~BoostContextFactory() override;
  Context* create_context(std::function<void()> code,
    void_pfn_smxprocess_t, smx_actor_t process) override;
  void run_all() override;
};

}}} // namespace

#endif
