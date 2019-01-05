/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file UContext.cpp Context switching with ucontexts from System V        */

#include "context_private.hpp"

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "src/mc/mc_ignore.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.hpp"

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
  UContext::set_maestro(nullptr);
  if (parallel_)
    SwappedContext::initialize();
}

UContextFactory::~UContextFactory()
{
  if (parallel_)
    SwappedContext::finalize();
}

Context* UContextFactory::create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process)
{
  if (parallel_)
    return new_context<ParallelUContext>(std::move(code), cleanup, process);
  else
    return new_context<UContext>(std::move(code), cleanup, process);
}

/* This function is called by maestro at the beginning of a scheduling round to get all working threads executing some
 * stuff It is much easier to understand what happens if you see the working threads as bodies that swap their soul for
 * the ones of the simulated processes that must run.
 */
void UContextFactory::run_all()
{
  if (parallel_)
    ParallelUContext::run_all();
  else
    SwappedContext::run_all();
}

// UContext

UContext::UContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
    : SwappedContext(std::move(code), cleanup_func, process)
{
  /* if the user provided a function for the process then use it, otherwise it is the context for maestro */
  if (has_code()) {
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
    if (process != nullptr && get_maestro() == nullptr)
      set_maestro(this);
  }

#if SIMGRID_HAVE_MC
  if (MC_is_active() && has_code()) {
    MC_register_stack_area(this->stack_, process, &(this->uc_), smx_context_usable_stack_size);
  }
#endif
}

UContext::~UContext() = default;

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

void UContext::swap_into(SwappedContext* to_)
{
  UContext* to = static_cast<UContext*>(to_);
  ASAN_ONLY(void* fake_stack = nullptr);
  ASAN_ONLY(to->asan_ctx_ = from);
  ASAN_START_SWITCH(from->asan_stop_ ? nullptr : &fake_stack, to->asan_stack_, to->asan_stack_size_);
  swapcontext(&this->uc_, &to->uc_);
  ASAN_FINISH_SWITCH(fake_stack, &from->asan_ctx_->asan_stack_, &from->asan_ctx_->asan_stack_size_);
}

void UContext::stop()
{
  Context::stop();
  throw StopRequest();
}

// ParallelUContext

void ParallelUContext::run_all()
{
  threads_working_ = 0;

  // We lazily create the parmap so that all options are actually processed when doing so.
  if (parmap_ == nullptr)
    parmap_ = new simgrid::xbt::Parmap<smx_actor_t>(SIMIX_context_get_nthreads(), SIMIX_context_get_parallel_mode());

  // Usually, Parmap::apply() executes the provided function on all elements of the array.
  // Here, the executed function does not return the control to the parmap before all the array is processed:
  //   - suspend() should switch back to the worker_context (either maestro or one of its minions) to return
  //     the control to the parmap. Instead, it uses parmap_->next() to steal another work, and does it directly.
  //     It only yields back to worker_context when the work array is exhausted.
  //   - So, resume() is only launched from the parmap for the first job of each minion.
  parmap_->apply(
      [](smx_actor_t process) {
        ParallelUContext* context = static_cast<ParallelUContext*>(process->context_);
        context->resume();
      },
      simix_global->process_to_run);
}

/** Run one particular simulated process on the current thread.
 *
 * Only applied to the N first elements of the parmap array, where N is the amount of worker threads in the parmap.
 * See ParallelUContext::run_all for details.
 */
void ParallelUContext::resume()
{
  // Save the thread number (my body) in an os-thread-specific area
  worker_id_ = threads_working_.fetch_add(1, std::memory_order_relaxed);
  // Save my current soul (either maestro, or one of the minions) in a permanent area
  SwappedContext* worker_context = static_cast<SwappedContext*>(self());
  workers_context_[worker_id_]   = worker_context;
  // Switch my soul and the actor's one
  Context::set_current(this);
  worker_context->swap_into(this);
  // No body runs that soul anymore at this point, but it is stored in a safe place.
  // When the executed actor will do a blocking action, SIMIX_process_yield() will call suspend(), below.
}

/** Yield
 *
 * This function is called when a simulated process wants to yield back to the maestro in a blocking simcall,
 * ie in SIMIX_process_yield().
 *
 * Actually, it does not really yield back to maestro, but directly into the next executable actor.
 *
 * This makes the parmap::apply awkward (see ParallelUContext::run_all()) because it only apply regularly
 * on the few first elements of the array, but it saves a lot of context switches back to maestro,
 * and directly forth to the next executable actor.
 */
void ParallelUContext::suspend()
{
  // Get some more work to directly swap into the next executable actor instead of yielding back to the parmap
  boost::optional<smx_actor_t> next_work = parmap_->next();
  SwappedContext* next_context;
  if (next_work) {
    // There is a next soul to embody (ie, another executable actor)
    XBT_DEBUG("Run next process");
    next_context = static_cast<ParallelUContext*>(next_work.get()->context_);
  } else {
    // All actors were run, go back to the parmap context
    XBT_DEBUG("No more processes to run");
    // worker_id_ is the identity of my body, stored in thread_local when starting the scheduling round
    next_context = workers_context_[worker_id_];
    // When given that soul, the body will wait for the next scheduling round
  }

  // Get the next soul to run, either from another actor or the initial minion's one
  Context::set_current(next_context);
  this->swap_into(next_context);
}

XBT_PRIVATE ContextFactory* sysv_factory()
{
  XBT_VERB("Activating SYSV context factory");
  return new UContextFactory();
}
}}} // namespace simgrid::kernel::context
