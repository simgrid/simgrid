/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdint>

#include <functional>
#include <utility>
#include <vector>

#include <boost/context/all.hpp>

#include <xbt/log.h>
#include <xbt/xbt_os_thread.h>

#include "src/simix/smx_private.h"
#include "src/internal_config.h"
#include "src/kernel/context/ContextBoost.hpp"

#if HAVE_SANITIZE_ADDRESS_FIBER_SUPPORT
#include <sanitizer/asan_interface.h>
#define ASAN_EVAL(expr) (expr)
#define ASAN_START_SWITCH(fake_stack_save, bottom, size) __sanitizer_start_switch_fiber(fake_stack_save, bottom, size)
#define ASAN_FINISH_SWITCH(fake_stack_save, bottom_old, size_old)                                                      \
  __sanitizer_finish_switch_fiber(fake_stack_save, bottom_old, size_old)
#else
#define ASAN_EVAL(expr) (void)0
#define ASAN_START_SWITCH(fake_stack_save, bottom, size) (void)0
#define ASAN_FINISH_SWITCH(fake_stack_save, bottom_old, size_old) (void)(fake_stack_save)
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

namespace simgrid {
namespace kernel {
namespace context {

class BoostSerialContext : public BoostContext {
public:
  BoostSerialContext(std::function<void()> code,
      void_pfn_smxprocess_t cleanup_func,
      smx_actor_t process)
    : BoostContext(std::move(code), cleanup_func, process) {}
  void suspend() override;
};

#if HAVE_THREAD_CONTEXTS
class BoostParallelContext : public BoostContext {
public:
  BoostParallelContext(std::function<void()> code,
      void_pfn_smxprocess_t cleanup_func,
      smx_actor_t process)
    : BoostContext(std::move(code), cleanup_func, process) {}
  void suspend() override;
  void resume() override;
};
#endif

// BoostContextFactory

bool BoostContext::parallel_                             = false;
simgrid::xbt::Parmap<smx_actor_t>* BoostContext::parmap_ = nullptr;
uintptr_t BoostContext::threads_working_                 = 0;
xbt_os_thread_key_t BoostContext::worker_id_key_;
unsigned long BoostContext::process_index_   = 0;
BoostContext* BoostContext::maestro_context_ = nullptr;
std::vector<BoostContext*> BoostContext::workers_context_;

BoostContextFactory::BoostContextFactory()
  : ContextFactory("BoostContextFactory")
{
  BoostContext::parallel_ = SIMIX_context_is_parallel();
  if (BoostContext::parallel_) {
#if HAVE_THREAD_CONTEXTS
    BoostContext::parmap_ = nullptr;
    BoostContext::workers_context_.clear();
    BoostContext::workers_context_.resize(SIMIX_context_get_nthreads(), nullptr);
    BoostContext::maestro_context_ = nullptr;
    xbt_os_thread_key_create(&BoostContext::worker_id_key_);
#else
    xbt_die("No thread support for parallel context execution");
#endif
  }
}

BoostContextFactory::~BoostContextFactory()
{
#if HAVE_THREAD_CONTEXTS
  delete BoostContext::parmap_;
  BoostContext::parmap_ = nullptr;
  BoostContext::workers_context_.clear();
#endif
}

smx_context_t BoostContextFactory::create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup_func,
                                                  smx_actor_t process)
{
  BoostContext* context = nullptr;
  if (BoostContext::parallel_)
#if HAVE_THREAD_CONTEXTS
    context = this->new_context<BoostParallelContext>(std::move(code), cleanup_func, process);
#else
    xbt_die("No support for parallel execution");
#endif
  else
    context = this->new_context<BoostSerialContext>(std::move(code), cleanup_func, process);
  return context;
}

void BoostContextFactory::run_all()
{
#if HAVE_THREAD_CONTEXTS
  if (BoostContext::parallel_) {
    BoostContext::threads_working_ = 0;
    if (not BoostContext::parmap_)
      BoostContext::parmap_ =
          new simgrid::xbt::Parmap<smx_actor_t>(SIMIX_context_get_nthreads(), SIMIX_context_get_parallel_mode());
    BoostContext::parmap_->apply(
        [](smx_actor_t process) {
          BoostContext* context = static_cast<BoostContext*>(process->context);
          return context->resume();
        },
        simix_global->process_to_run);
  } else
#endif
  {
    if (simix_global->process_to_run.empty())
      return;
    smx_actor_t first_process    = simix_global->process_to_run.front();
    BoostContext::process_index_ = 1;
    /* execute the first process */
    static_cast<BoostContext*>(first_process->context)->resume();
  }
}


// BoostContext

void BoostContext::smx_ctx_boost_wrapper(BoostContext::ctx_arg_type arg)
{
#if BOOST_VERSION < 106100
  BoostContext* context = reinterpret_cast<BoostContext*>(arg);
#else
  ASAN_FINISH_SWITCH(nullptr, &static_cast<BoostContext**>(arg.data)[0]->asan_stack_,
                     &static_cast<BoostContext**>(arg.data)[0]->asan_stack_size_);
  static_cast<BoostContext**>(arg.data)[0]->fc_ = arg.fctx;
  BoostContext* context                         = static_cast<BoostContext**>(arg.data)[1];
#endif
  try {
    (*context)();
    context->Context::stop();
  } catch (StopRequest const&) {
    XBT_DEBUG("Caught a StopRequest");
  }
  ASAN_EVAL(context->asan_stop_ = true);
  context->suspend();
}

inline void BoostContext::smx_ctx_boost_jump_fcontext(BoostContext* from, BoostContext* to)
{
#if BOOST_VERSION < 105600
  boost::context::jump_fcontext(from->fc_, to->fc_, reinterpret_cast<intptr_t>(to));
#elif BOOST_VERSION < 106100
  boost::context::jump_fcontext(&from->fc_, to->fc_, reinterpret_cast<intptr_t>(to));
#else
  BoostContext* ctx[2] = {from, to};
  void* fake_stack;
  ASAN_START_SWITCH(from->asan_stop_ ? nullptr : &fake_stack, to->asan_stack_, to->asan_stack_size_);
  boost::context::detail::transfer_t arg = boost::context::detail::jump_fcontext(to->fc_, ctx);
  ASAN_FINISH_SWITCH(fake_stack, &static_cast<BoostContext**>(arg.data)[0]->asan_stack_,
                     &static_cast<BoostContext**>(arg.data)[0]->asan_stack_size_);
  static_cast<BoostContext**>(arg.data)[0]->fc_ = arg.fctx;
#endif
}

BoostContext::BoostContext(std::function<void()> code,
    void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
  : Context(std::move(code), cleanup_func, process)
{

  /* if the user provided a function for the process then use it, otherwise it is the context for maestro */
  if (has_code()) {
    this->stack_ = SIMIX_context_stack_new();
// We need to pass the bottom of the stack to make_fcontext, depending on the stack direction it may be the lower
// or higher address:
#if PTH_STACKGROWTH == -1
    void* stack = static_cast<char*>(this->stack_) + smx_context_usable_stack_size;
#else
    void* stack = this->stack_;
#endif
    ASAN_EVAL(this->asan_stack_ = stack);
#if BOOST_VERSION < 106100
    this->fc_ = boost::context::make_fcontext(stack, smx_context_usable_stack_size, smx_ctx_boost_wrapper);
#else
    this->fc_ = boost::context::detail::make_fcontext(stack, smx_context_usable_stack_size, smx_ctx_boost_wrapper);
#endif
  } else {
#if BOOST_VERSION < 105600
    this->fc_ = new boost::context::fcontext_t();
#endif
    if (BoostContext::maestro_context_ == nullptr)
      BoostContext::maestro_context_ = this;
  }
}

BoostContext::~BoostContext()
{
#if BOOST_VERSION < 105600
  if (not this->stack_)
    delete this->fc_;
#endif
  if (this == maestro_context_)
    maestro_context_ = nullptr;
  SIMIX_context_stack_delete(this->stack_);
}

// BoostSerialContext

void BoostContext::stop()
{
  Context::stop();
  throw StopRequest();
}

void BoostContext::resume()
{
  SIMIX_context_set_current(this);
  smx_ctx_boost_jump_fcontext(maestro_context_, this);
}

void BoostSerialContext::suspend()
{
  /* determine the next context */
  BoostSerialContext* next_context;
  unsigned long int i = process_index_;
  process_index_++;

  if (i < simix_global->process_to_run.size()) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = static_cast<BoostSerialContext*>(simix_global->process_to_run[i]->context);
  } else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = static_cast<BoostSerialContext*>(maestro_context_);
  }
  SIMIX_context_set_current(static_cast<smx_context_t>(next_context));
  smx_ctx_boost_jump_fcontext(this, next_context);
}

// BoostParallelContext

#if HAVE_THREAD_CONTEXTS

void BoostParallelContext::suspend()
{
  boost::optional<smx_actor_t> next_work = parmap_->next();
  BoostParallelContext* next_context;
  if (next_work) {
    XBT_DEBUG("Run next process");
    next_context = static_cast<BoostParallelContext*>(next_work.get()->context);
  } else {
    XBT_DEBUG("No more processes to run");
    uintptr_t worker_id = reinterpret_cast<uintptr_t>(xbt_os_thread_get_specific(worker_id_key_));
    next_context = static_cast<BoostParallelContext*>(workers_context_[worker_id]);
  }

  SIMIX_context_set_current(static_cast<smx_context_t>(next_context));
  smx_ctx_boost_jump_fcontext(this, next_context);
}

void BoostParallelContext::resume()
{
  uintptr_t worker_id = __sync_fetch_and_add(&threads_working_, 1);
  xbt_os_thread_set_specific(worker_id_key_, reinterpret_cast<void*>(worker_id));

  BoostParallelContext* worker_context = static_cast<BoostParallelContext*>(SIMIX_context_self());
  workers_context_[worker_id] = worker_context;

  SIMIX_context_set_current(this);
  smx_ctx_boost_jump_fcontext(worker_context, this);
}

#endif

XBT_PRIVATE ContextFactory* boost_factory()
{
  XBT_VERB("Using Boost contexts. Welcome to the 21th century.");
  return new BoostContextFactory();
}

}}} // namespace
