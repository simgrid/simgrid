/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @file BoostContext.cpp Userspace context switching implementation based on Boost.Context */

#include <cstdint>

#include <functional>
#include <utility>
#include <vector>

#include <boost/context/all.hpp>

#include <xbt/log.h>
#include <xbt/xbt_os_thread.h>

#include "smx_private.h"
#include "smx_private.hpp"
#include "src/internal_config.h"
#include "src/simix/BoostContext.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

namespace simgrid {
namespace simix {

class BoostSerialContext : public BoostContext {
public:
  BoostSerialContext(std::function<void()> code,
      void_pfn_smxprocess_t cleanup_func,
      smx_process_t process)
    : BoostContext(std::move(code), cleanup_func, process) {}
  void stop() override;
  void suspend() override;
  void resume();
};

#if HAVE_THREAD_CONTEXTS
class BoostParallelContext : public BoostContext {
public:
  BoostParallelContext(std::function<void()> code,
      void_pfn_smxprocess_t cleanup_func,
      smx_process_t process)
    : BoostContext(std::move(code), cleanup_func, process) {}
  void stop() override;
  void suspend() override;
  void resume();
};
#endif

// BoostContextFactory

bool                BoostContext::parallel_        = false;
xbt_parmap_t        BoostContext::parmap_          = nullptr;
uintptr_t           BoostContext::threads_working_ = 0;
xbt_os_thread_key_t BoostContext::worker_id_key_;
unsigned long       BoostContext::process_index_   = 0;
BoostContext*       BoostContext::maestro_context_ = nullptr;
std::vector<BoostContext*> BoostContext::workers_context_;

BoostContextFactory::BoostContextFactory()
  : ContextFactory("BoostContextFactory")
{
  BoostContext::parallel_ = SIMIX_context_is_parallel();
  if (BoostContext::parallel_) {
#if !HAVE_THREAD_CONTEXTS
    xbt_die("No thread support for parallel context execution");
#else
    int nthreads = SIMIX_context_get_nthreads();
    BoostContext::parmap_ = xbt_parmap_new(nthreads, SIMIX_context_get_parallel_mode());
    BoostContext::workers_context_.clear();
    BoostContext::workers_context_.resize(nthreads, nullptr);
    BoostContext::maestro_context_ = nullptr;
    xbt_os_thread_key_create(&BoostContext::worker_id_key_);
#endif
  }
}

BoostContextFactory::~BoostContextFactory()
{
#if HAVE_THREAD_CONTEXTS
  if (BoostContext::parmap_) {
    xbt_parmap_destroy(BoostContext::parmap_);
    BoostContext::parmap_ = nullptr;
  }
  BoostContext::workers_context_.clear();
#endif
}

smx_context_t BoostContextFactory::create_context(std::function<void()>  code,
  void_pfn_smxprocess_t cleanup_func, smx_process_t process)
{
  BoostContext* context = nullptr;
  if (BoostContext::parallel_)
#if HAVE_THREAD_CONTEXTS
    context = this->new_context<BoostParallelContext>(
      std::move(code), cleanup_func, process);
#else
    xbt_die("No support for parallel execution");
#endif
  else
    context = this->new_context<BoostSerialContext>(
      std::move(code), cleanup_func, process);
  return context;
}

void BoostContextFactory::run_all()
{
#if HAVE_THREAD_CONTEXTS
  if (BoostContext::parallel_) {
    BoostContext::threads_working_ = 0;
    xbt_parmap_apply(BoostContext::parmap_,
      [](void* arg) {
        smx_process_t process = static_cast<smx_process_t>(arg);
        BoostContext* context  = static_cast<BoostContext*>(process->context);
        return context->resume();
      },
      simix_global->process_to_run);
  } else
#endif
  {
    smx_process_t first_process =
        xbt_dynar_get_as(simix_global->process_to_run, 0, smx_process_t);
    BoostContext::process_index_ = 1;
    /* execute the first process */
    static_cast<BoostContext*>(first_process->context)->resume();
  }
}


// BoostContext

static void smx_ctx_boost_wrapper(std::intptr_t arg)
{
  BoostContext* context = (BoostContext*) arg;
  (*context)();
  context->stop();
}

BoostContext::BoostContext(std::function<void()> code,
    void_pfn_smxprocess_t cleanup_func, smx_process_t process)
  : Context(std::move(code), cleanup_func, process)
{

  /* if the user provided a function for the process then use it,
     otherwise it is the context for maestro */
  if (has_code()) {
    this->stack_ = SIMIX_context_stack_new();
    // We need to pass the bottom of the stack to make_fcontext,
    // depending on the stack direction it may be the lower or higher address:
  #if PTH_STACKGROWTH == -1
    void* stack = (char*) this->stack_ + smx_context_usable_stack_size - 1;
  #else
    void* stack = this->stack_;
  #endif
    this->fc_ = boost::context::make_fcontext(
                      stack,
                      smx_context_usable_stack_size,
                      smx_ctx_boost_wrapper);
  } else {
    #if HAVE_BOOST_CONTEXTS == 1
    this->fc_ = new boost::context::fcontext_t();
    #endif
    if (BoostContext::maestro_context_ == nullptr)
      BoostContext::maestro_context_ = this;
  }
}

BoostContext::~BoostContext()
{
#if HAVE_BOOST_CONTEXTS == 1
  if (!this->stack_)
    delete this->fc_;
#endif
  if (this == maestro_context_)
    maestro_context_ = nullptr;
  SIMIX_context_stack_delete(this->stack_);
}

// BoostSerialContext

void BoostContext::resume()
{
  SIMIX_context_set_current(this);
#if HAVE_BOOST_CONTEXTS == 1
  boost::context::jump_fcontext(
    maestro_context_->fc_, this->fc_,
    (intptr_t) this);
#else
  boost::context::jump_fcontext(
    &maestro_context_->fc_, this->fc_,
    (intptr_t) this);
#endif
}

void BoostSerialContext::suspend()
{
  /* determine the next context */
  BoostSerialContext* next_context = nullptr;
  unsigned long int i = process_index_++;

  if (i < xbt_dynar_length(simix_global->process_to_run)) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = static_cast<BoostSerialContext*>(xbt_dynar_get_as(
        simix_global->process_to_run, i, smx_process_t)->context);
  }
  else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = static_cast<BoostSerialContext*>(
      maestro_context_);
  }
  SIMIX_context_set_current((smx_context_t) next_context);
  #if HAVE_BOOST_CONTEXTS == 1
  boost::context::jump_fcontext(
    this->fc_, next_context->fc_, (intptr_t) next_context);
  #else
  boost::context::jump_fcontext(
    &this->fc_, next_context->fc_, (intptr_t) next_context);
  #endif
}

void BoostSerialContext::stop()
{
  BoostContext::stop();
  this->suspend();
}

// BoostParallelContext

#if HAVE_THREAD_CONTEXTS

void BoostParallelContext::suspend()
{
  smx_process_t next_work = (smx_process_t) xbt_parmap_next(parmap_);
  BoostParallelContext* next_context = nullptr;

  if (next_work != nullptr) {
    XBT_DEBUG("Run next process");
    next_context = static_cast<BoostParallelContext*>(next_work->context);
  }
  else {
    XBT_DEBUG("No more processes to run");
    uintptr_t worker_id =
      (uintptr_t) xbt_os_thread_get_specific(worker_id_key_);
    next_context = static_cast<BoostParallelContext*>(
      workers_context_[worker_id]);
  }

  SIMIX_context_set_current((smx_context_t) next_context);
#if HAVE_BOOST_CONTEXTS == 1
  boost::context::jump_fcontext(
    this->fc_, next_context->fc_, (intptr_t)next_context);
#else
  boost::context::jump_fcontext(
    &this->fc_, next_context->fc_, (intptr_t)next_context);
#endif
}

void BoostParallelContext::stop()
{
  BoostContext::stop();
  this->suspend();
}

void BoostParallelContext::resume()
{
  uintptr_t worker_id = __sync_fetch_and_add(&threads_working_, 1);
  xbt_os_thread_set_specific(worker_id_key_, (void*) worker_id);

  BoostParallelContext* worker_context =
    static_cast<BoostParallelContext*>(SIMIX_context_self());
  workers_context_[worker_id] = worker_context;

  SIMIX_context_set_current(this);
#if HAVE_BOOST_CONTEXTS == 1
  boost::context::jump_fcontext(
    worker_context->fc_, this->fc_, (intptr_t) this);
#else
  boost::context::jump_fcontext(
    &worker_context->fc_, this->fc_, (intptr_t) this);
#endif
}

#endif

XBT_PRIVATE ContextFactory* boost_factory()
{
  XBT_VERB("Using Boost contexts. Welcome to the 21th century.");
  return new BoostContextFactory();
}

}
}
