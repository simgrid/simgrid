/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "ContextBoost.hpp"

#include <utility>
#include <xbt/log.h>

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

// BoostContextFactory

BoostContextFactory::BoostContextFactory()
    : ContextFactory("BoostContextFactory"), parallel_(SIMIX_context_is_parallel())
{
  BoostContext::setMaestro(nullptr);
  if (parallel_) {
#if HAVE_THREAD_CONTEXTS
    ParallelBoostContext::initialize();
#else
    xbt_die("No thread support for parallel context execution");
#endif
  }
}

BoostContextFactory::~BoostContextFactory()
{
#if HAVE_THREAD_CONTEXTS
  if (parallel_)
    ParallelBoostContext::finalize();
#endif
}

smx_context_t BoostContextFactory::create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup_func,
                                                  smx_actor_t process)
{
#if HAVE_THREAD_CONTEXTS
  if (parallel_)
    return this->new_context<ParallelBoostContext>(std::move(code), cleanup_func, process);
#endif

  return this->new_context<SerialBoostContext>(std::move(code), cleanup_func, process);
}

void BoostContextFactory::run_all()
{
#if HAVE_THREAD_CONTEXTS
  if (parallel_)
    ParallelBoostContext::run_all();
  else
#endif
    SerialBoostContext::run_all();
}

// BoostContext

BoostContext* BoostContext::maestro_context_ = nullptr;

BoostContext::BoostContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
    : Context(std::move(code), cleanup_func, process)
{

  /* if the user provided a function for the process then use it, otherwise it is the context for maestro */
  if (has_code()) {
    this->stack_ = SIMIX_context_stack_new();
    /* We need to pass the bottom of the stack to make_fcontext,
       depending on the stack direction it may be the lower or higher address: */
#if PTH_STACKGROWTH == -1
    void* stack = static_cast<char*>(this->stack_) + smx_context_usable_stack_size;
#else
    void* stack = this->stack_;
#endif
    ASAN_EVAL(this->asan_stack_ = stack);
#if BOOST_VERSION < 106100
    this->fc_ = boost::context::make_fcontext(stack, smx_context_usable_stack_size, BoostContext::wrapper);
#else
    this->fc_ = boost::context::detail::make_fcontext(stack, smx_context_usable_stack_size, BoostContext::wrapper);
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

void BoostContext::wrapper(BoostContext::arg_type arg)
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

inline void BoostContext::swap(BoostContext* from, BoostContext* to)
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

void BoostContext::stop()
{
  Context::stop();
  throw StopRequest();
}

// SerialBoostContext

unsigned long SerialBoostContext::process_index_;

void SerialBoostContext::suspend()
{
  /* determine the next context */
  SerialBoostContext* next_context;
  unsigned long int i = process_index_;
  process_index_++;

  if (i < simix_global->process_to_run.size()) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = static_cast<SerialBoostContext*>(simix_global->process_to_run[i]->context);
  } else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = static_cast<SerialBoostContext*>(BoostContext::getMaestro());
  }
  SIMIX_context_set_current(next_context);
  BoostContext::swap(this, next_context);
}

void SerialBoostContext::resume()
{
  SIMIX_context_set_current(this);
  BoostContext::swap(BoostContext::getMaestro(), this);
}

void SerialBoostContext::run_all()
{
  if (simix_global->process_to_run.empty())
    return;
  smx_actor_t first_process = simix_global->process_to_run.front();
  process_index_            = 1;
  /* execute the first process */
  static_cast<SerialBoostContext*>(first_process->context)->resume();
}

// ParallelBoostContext

#if HAVE_THREAD_CONTEXTS

simgrid::xbt::Parmap<smx_actor_t>* ParallelBoostContext::parmap_;
std::atomic<uintptr_t> ParallelBoostContext::threads_working_;
xbt_os_thread_key_t ParallelBoostContext::worker_id_key_;
std::vector<ParallelBoostContext*> ParallelBoostContext::workers_context_;

void ParallelBoostContext::initialize()
{
  parmap_ = nullptr;
  workers_context_.clear();
  workers_context_.resize(SIMIX_context_get_nthreads(), nullptr);
  xbt_os_thread_key_create(&worker_id_key_);
}

void ParallelBoostContext::finalize()
{
  delete parmap_;
  parmap_ = nullptr;
  workers_context_.clear();
  xbt_os_thread_key_destroy(worker_id_key_);
}

void ParallelBoostContext::run_all()
{
  threads_working_ = 0;
  if (parmap_ == nullptr)
    parmap_ = new simgrid::xbt::Parmap<smx_actor_t>(SIMIX_context_get_nthreads(), SIMIX_context_get_parallel_mode());
  parmap_->apply(
      [](smx_actor_t process) {
        ParallelBoostContext* context = static_cast<ParallelBoostContext*>(process->context);
        context->resume();
      },
      simix_global->process_to_run);
}

void ParallelBoostContext::suspend()
{
  boost::optional<smx_actor_t> next_work = parmap_->next();
  ParallelBoostContext* next_context;
  if (next_work) {
    XBT_DEBUG("Run next process");
    next_context = static_cast<ParallelBoostContext*>(next_work.get()->context);
  } else {
    XBT_DEBUG("No more processes to run");
    uintptr_t worker_id = reinterpret_cast<uintptr_t>(xbt_os_thread_get_specific(worker_id_key_));
    next_context        = workers_context_[worker_id];
  }

  SIMIX_context_set_current(next_context);
  BoostContext::swap(this, next_context);
}

void ParallelBoostContext::resume()
{
  uintptr_t worker_id = threads_working_.fetch_add(1, std::memory_order_relaxed);
  xbt_os_thread_set_specific(worker_id_key_, reinterpret_cast<void*>(worker_id));

  ParallelBoostContext* worker_context = static_cast<ParallelBoostContext*>(SIMIX_context_self());
  workers_context_[worker_id]          = worker_context;

  SIMIX_context_set_current(this);
  BoostContext::swap(worker_context, this);
}

#endif

XBT_PRIVATE ContextFactory* boost_factory()
{
  XBT_VERB("Using Boost contexts. Welcome to the 21th century.");
  return new BoostContextFactory();
}
}}} // namespace
