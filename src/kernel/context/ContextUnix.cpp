/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file UContext.cpp Context switching with ucontexts from System V        */

#include "context_private.hpp"

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "src/mc/mc_ignore.hpp"
#include "src/simix/ActorImpl.hpp"

#include "ContextUnix.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

/** Many integers are needed to store a pointer
 *
 * Support up to two ints. */
constexpr int CTX_ADDR_LEN = 2;

static_assert(sizeof(simgrid::kernel::context::UContext*) <= CTX_ADDR_LEN * sizeof(int),
              "Ucontexts are not supported on this arch yet");

namespace simgrid {
namespace kernel {
namespace context {

// UContextFactory

UContextFactory::UContextFactory() : ContextFactory("UContextFactory"), parallel_(SIMIX_context_is_parallel())
{
  UContext::setMaestro(nullptr);
  if (parallel_) {
#if HAVE_THREAD_CONTEXTS
    ParallelUContext::initialize();
#else
    xbt_die("No thread support for parallel context execution");
#endif
  }
}

UContextFactory::~UContextFactory()
{
#if HAVE_THREAD_CONTEXTS
  if (parallel_)
    ParallelUContext::finalize();
#endif
}

Context* UContextFactory::create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process)
{
#if HAVE_THREAD_CONTEXTS
  if (parallel_)
    return new_context<ParallelUContext>(std::move(code), cleanup, process);
  else
#endif
    return new_context<SerialUContext>(std::move(code), cleanup, process);
}

/* This function is called by maestro at the beginning of a scheduling round to get all working threads executing some
 * stuff It is much easier to understand what happens if you see the working threads as bodies that swap their soul for
 * the ones of the simulated processes that must run.
 */
void UContextFactory::run_all()
{
#if HAVE_THREAD_CONTEXTS
  if (parallel_)
    ParallelUContext::run_all();
  else
#endif
    SerialUContext::run_all();
}

// UContext

UContext* UContext::maestro_context_ = nullptr;

UContext::UContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
    : Context(std::move(code), cleanup_func, process)
{
  /* if the user provided a function for the process then use it, otherwise it is the context for maestro */
  if (has_code()) {
    this->stack_ = SIMIX_context_stack_new();
    getcontext(&this->uc_);
    this->uc_.uc_link = nullptr;
    this->uc_.uc_stack.ss_sp   = sg_makecontext_stack_addr(this->stack_);
    this->uc_.uc_stack.ss_size = sg_makecontext_stack_size(smx_context_usable_stack_size);
#if PTH_STACKGROWTH == -1
    ASAN_ONLY(this->asan_stack_ = static_cast<char*>(this->stack_) + smx_context_usable_stack_size);
#else
    ASAN_ONLY(this->asan_stack_ = this->stack_);
#endif
    UContext::make_ctx(&this->uc_, UContext::smx_ctx_sysv_wrapper, this);
  } else {
    if (process != nullptr && maestro_context_ == nullptr)
      maestro_context_ = this;
  }

#if SIMGRID_HAVE_MC
  if (MC_is_active() && has_code()) {
    MC_register_stack_area(this->stack_, process, &(this->uc_), smx_context_usable_stack_size);
  }
#endif
}

UContext::~UContext()
{
  SIMIX_context_stack_delete(this->stack_);
}

// The name of this function is currently hardcoded in the code (as string).
// Do not change it without fixing those references as well.
void UContext::smx_ctx_sysv_wrapper(int i1, int i2)
{
  // Rebuild the Context* pointer from the integers:
  int ctx_addr[CTX_ADDR_LEN] = {i1, i2};
  simgrid::kernel::context::UContext* context;
  memcpy(&context, ctx_addr, sizeof context);

  ASAN_FINISH_SWITCH(nullptr, &context->asan_ctx_->asan_stack_, &context->asan_ctx_->asan_stack_size_);
  try {
    (*context)();
  } catch (simgrid::kernel::context::Context::StopRequest const&) {
    XBT_DEBUG("Caught a StopRequest");
  } catch (simgrid::Exception const& e) {
    XBT_INFO("Actor killed by an uncatched exception %s", simgrid::xbt::demangle(typeid(e).name()).get());
    throw;
  }
  context->Context::stop();
  ASAN_ONLY(context->asan_stop_ = true);
  context->suspend();
}

/** A better makecontext
 *
 * Makecontext expects integer arguments, we the context variable is decomposed into a serie of integers and each
 * integer is passed as argument to makecontext.
 */
void UContext::make_ctx(ucontext_t* ucp, void (*func)(int, int), UContext* arg)
{
  int ctx_addr[CTX_ADDR_LEN]{};
  memcpy(ctx_addr, &arg, sizeof arg);
  makecontext(ucp, (void (*)())func, 2, ctx_addr[0], ctx_addr[1]);
}

inline void UContext::swap(UContext* from, UContext* to)
{
  ASAN_ONLY(void* fake_stack = nullptr);
  ASAN_ONLY(to->asan_ctx_ = from);
  ASAN_START_SWITCH(from->asan_stop_ ? nullptr : &fake_stack, to->asan_stack_, to->asan_stack_size_);
  swapcontext(&from->uc_, &to->uc_);
  ASAN_FINISH_SWITCH(fake_stack, &from->asan_ctx_->asan_stack_, &from->asan_ctx_->asan_stack_size_);
}

void UContext::stop()
{
  Context::stop();
  throw StopRequest();
}

// SerialUContext

unsigned long SerialUContext::process_index_; /* index of the next process to run in the list of runnable processes */

void SerialUContext::suspend()
{
  /* determine the next context */
  SerialUContext* next_context;
  unsigned long int i = process_index_;
  process_index_++;

  if (i < simix_global->process_to_run.size()) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = static_cast<SerialUContext*>(simix_global->process_to_run[i]->context_);
  } else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = static_cast<SerialUContext*>(UContext::getMaestro());
  }
  SIMIX_context_set_current(next_context);
  UContext::swap(this, next_context);
}

void SerialUContext::resume()
{
  SIMIX_context_set_current(this);
  UContext::swap(UContext::getMaestro(), this);
}

void SerialUContext::run_all()
{
  if (simix_global->process_to_run.empty())
    return;
  smx_actor_t first_process = simix_global->process_to_run.front();
  process_index_            = 1;
  static_cast<SerialUContext*>(first_process->context_)->resume();
}

// ParallelUContext

#if HAVE_THREAD_CONTEXTS

simgrid::xbt::Parmap<smx_actor_t>* ParallelUContext::parmap_;
std::atomic<uintptr_t> ParallelUContext::threads_working_;         /* number of threads that have started their work */
thread_local uintptr_t ParallelUContext::worker_id_;               /* thread-specific storage for the thread id */
std::vector<ParallelUContext*> ParallelUContext::workers_context_; /* space to save the worker's context
                                                                    * in each thread */

void ParallelUContext::initialize()
{
  parmap_ = nullptr;
  workers_context_.clear();
  workers_context_.resize(SIMIX_context_get_nthreads(), nullptr);
}

void ParallelUContext::finalize()
{
  delete parmap_;
  parmap_ = nullptr;
  workers_context_.clear();
}

void ParallelUContext::run_all()
{
  threads_working_ = 0;
  // Parmap_apply ensures that every working thread get an index in the process_to_run array (through an atomic
  // fetch_and_add), and runs the ParallelUContext::resume function on that index

  // We lazily create the parmap because the parmap creates context with simix_global->context_factory (which might not
  // be initialized when bootstrapping):
  if (parmap_ == nullptr)
    parmap_ = new simgrid::xbt::Parmap<smx_actor_t>(SIMIX_context_get_nthreads(), SIMIX_context_get_parallel_mode());
  parmap_->apply(
      [](smx_actor_t process) {
        ParallelUContext* context = static_cast<ParallelUContext*>(process->context_);
        context->resume();
      },
      simix_global->process_to_run);
}

/** Yield
 *
 * This function is called when a simulated process wants to yield back to the maestro in a blocking simcall. This
 * naturally occurs within SIMIX_context_suspend(self->context), called from SIMIX_process_yield() Actually, it does not
 * really yield back to maestro, but into the next process that must be executed. If no one is to be executed, then it
 * yields to the initial soul that was in this working thread (that was saved in resume_parallel).
 */
void ParallelUContext::suspend()
{
  /* determine the next context */
  // Get the next soul to embody now:
  boost::optional<smx_actor_t> next_work = parmap_->next();
  ParallelUContext* next_context;
  if (next_work) {
    // There is a next soul to embody (ie, a next process to resume)
    XBT_DEBUG("Run next process");
    next_context = static_cast<ParallelUContext*>(next_work.get()->context_);
  } else {
    // All processes were run, go to the barrier
    XBT_DEBUG("No more processes to run");
    // worker_id_ is the identity of my body, stored in thread_local when starting the scheduling round
    // Deduce the initial soul of that body
    next_context = workers_context_[worker_id_];
    // When given that soul, the body will wait for the next scheduling round
  }

  SIMIX_context_set_current(next_context);
  // Get the next soul to run, either simulated or initial minion's one:
  UContext::swap(this, next_context);
}

/** Run one particular simulated process on the current thread. */
void ParallelUContext::resume()
{
  // What is my containing body? Store its number in os-thread-specific area :
  worker_id_ = threads_working_.fetch_add(1, std::memory_order_relaxed);
  // Get my current soul:
  ParallelUContext* worker_context = static_cast<ParallelUContext*>(SIMIX_context_self());
  // Write down that this soul is hosted in that body (for now)
  workers_context_[worker_id_] = worker_context;
  // Write in simix that I switched my soul
  SIMIX_context_set_current(this);
  // Actually do that using the relevant library call:
  UContext::swap(worker_context, this);
  // No body runs that soul anymore at this point.  Instead the current body took the soul of simulated process The
  // simulated process wakes back after the call to "SIMIX_context_suspend(self->context);" within
  // smx_process.c::SIMIX_process_yield()

  // From now on, the simulated processes will change their soul with the next soul to execute (in suspend_parallel,
  // below).  When nobody is to be executed in this scheduling round, the last simulated process will take back the
  // initial soul of the current working thread
}

#endif

XBT_PRIVATE ContextFactory* sysv_factory()
{
  XBT_VERB("Activating SYSV context factory");
  return new UContextFactory();
}
}}} // namespace simgrid::kernel::context
