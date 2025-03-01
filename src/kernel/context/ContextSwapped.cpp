/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/modelchecker.h"
#include "src/internal_config.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/sthread/sthread.h"
#include "src/xbt/parmap.hpp"

#include "src/kernel/context/ContextSwapped.hpp"

#include <boost/core/demangle.hpp>
#include <memory>
#include <sys/mman.h>
#include <typeinfo>

#if HAVE_VALGRIND_H
#include <valgrind/valgrind.h>
#endif
#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
#include <sanitizer/asan_interface.h>
#endif
#if HAVE_SANITIZER_THREAD_FIBER_SUPPORT
#include <sanitizer/tsan_interface.h>
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_context);

// The name of this function is currently hardcoded in MC (as string).
// Do not change it without fixing those references as well.
void smx_ctx_wrapper(simgrid::kernel::context::SwappedContext* context)
{
#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
  __sanitizer_finish_switch_fiber(nullptr, &context->asan_ctx_->asan_stack_, &context->asan_ctx_->asan_stack_size_);
#endif
  try {
    sthread_enable();
    (*context)();
    sthread_disable();
    context->stop();
  } catch (simgrid::ForcefulKillException const&) {
    sthread_disable();
    XBT_DEBUG("Caught a ForcefulKillException");
  } catch (simgrid::Exception const& e) {
    sthread_disable();
    XBT_INFO("Actor killed by an uncaught exception %s", boost::core::demangle(typeid(e).name()).c_str());
    throw;
  }
#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
  context->asan_stop_ = true;
#endif
  context->suspend();
  THROW_IMPOSSIBLE;
}

namespace simgrid::kernel::context {

/* thread-specific storage for the worker's context */
thread_local SwappedContext* SwappedContext::worker_context_ = nullptr;

SwappedContext::SwappedContext(std::function<void()>&& code, actor::ActorImpl* actor, SwappedContextFactory* factory)
    : Context(std::move(code), actor, not code /* maestro if no code */), factory_(*factory)
{
  // Save maestro (=first created context) in preparation for run_all
  if (not is_parallel() && factory_.maestro_context_ == nullptr)
    factory_.maestro_context_ = this;

  if (has_code()) {
    xbt_assert((actor->get_stacksize() & 0xf) == 0, "Actor stack size should be multiple of 16");
    if (guard_size > 0 && not MC_is_active()) {
#if PTH_STACKGROWTH != -1
      xbt_die(
          "Stack overflow protection is known to be broken on your system: you stacks grow upwards (or detection is "
          "broken). "
          "Please disable stack guards with --cfg=contexts:guard-size:0");
      /* Current code for stack overflow protection assumes that stacks are growing downward (PTH_STACKGROWTH == -1).
       * Protected pages need to be put after the stack when PTH_STACKGROWTH == 1. */
#endif

      size_t size = actor->get_stacksize() + guard_size;
      void* alloc;
      xbt_assert(posix_memalign(&alloc, xbt_pagesize, size) == 0, "Failed to allocate stack.");
      this->stack_ = static_cast<unsigned char*>(alloc);

      /* This is fatal. We are going to fail at some point when we try reusing this. */
      xbt_assert(
          mprotect(this->stack_, guard_size, PROT_NONE) != -1,
          "Failed to protect stack: %s.\n"
          "If you are running a lot of actors, you may be exceeding the amount of mappings allowed per process.\n"
          "On Linux systems, change this value with sudo sysctl -w vm.max_map_count=newvalue (default value: 65536)\n"
          "Please see https://simgrid.org/doc/latest/Configuring_SimGrid.html#configuring-the-user-code-virtualization "
          "for more information.",
          strerror(errno));

      this->stack_ = this->stack_ + guard_size;
    } else {
      this->stack_ = static_cast<unsigned char*>(xbt_malloc0(actor->get_stacksize()));
    }

#if HAVE_VALGRIND_H
    if (RUNNING_ON_VALGRIND)
      this->valgrind_stack_id_ = VALGRIND_STACK_REGISTER(this->stack_, this->stack_ + actor->get_stacksize());
#endif
#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
    this->asan_stack_ = get_stack_bottom();
#endif
#if HAVE_SANITIZER_THREAD_FIBER_SUPPORT
    this->tsan_fiber_ = __tsan_create_fiber(0);
#endif
  } else {
    // not has_code(): in maestro context
#if HAVE_SANITIZER_THREAD_FIBER_SUPPORT
    this->tsan_fiber_ = __tsan_get_current_fiber();
#endif
  }
}

SwappedContext::~SwappedContext()
{
  if (stack_ == nullptr) // maestro has no extra stack
    return;

#if HAVE_SANITIZER_THREAD_FIBER_SUPPORT
  __tsan_destroy_fiber(tsan_fiber_);
#endif
#if HAVE_VALGRIND_H
  if (valgrind_stack_id_ != 0)
    VALGRIND_STACK_DEREGISTER(valgrind_stack_id_);
#endif

  if (guard_size > 0 && not MC_is_active()) {
    stack_ = stack_ - guard_size;
    if (mprotect(stack_, guard_size, PROT_READ | PROT_WRITE) == -1) {
      XBT_WARN("Failed to remove page protection: %s", strerror(errno));
      /* try to pursue anyway */
    }
  }

  xbt_free(stack_);
}

unsigned char* SwappedContext::get_stack_bottom() const
{
  // Depending on the stack direction, its bottom (that make_fcontext needs) may be the lower or higher end
#if PTH_STACKGROWTH == 1
  return stack_;
#else
  return stack_ + get_actor()->get_stacksize();
#endif
}

void SwappedContext::swap_into(SwappedContext* to)
{
#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
  void* fake_stack = nullptr;
  to->asan_ctx_    = this;
  __sanitizer_start_switch_fiber(this->asan_stop_ ? nullptr : &fake_stack, to->asan_stack_, to->asan_stack_size_);
#endif
#if HAVE_SANITIZER_THREAD_FIBER_SUPPORT
  __tsan_switch_to_fiber(to->tsan_fiber_, 0);
#endif

  swap_into_for_real(to);

#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
  __sanitizer_finish_switch_fiber(fake_stack, &this->asan_ctx_->asan_stack_, &this->asan_ctx_->asan_stack_size_);
#endif
}

/** Maestro wants to run all ready actors */
void SwappedContextFactory::run_all(std::vector<actor::ActorImpl*> const& actors_list)
{
  const auto* engine = EngineImpl::get_instance();
  /* This function is called by maestro at the beginning of a scheduling round to get all working threads executing some
   * stuff It is much easier to understand what happens if you see the working threads as bodies that swap their soul
   * for the ones of the simulated processes that must run.
   */
  if (Context::is_parallel()) {
    // We lazily create the parmap so that all options are actually processed when doing so.
    if (parmap_ == nullptr)
      parmap_ =
          std::make_unique<simgrid::xbt::Parmap<actor::ActorImpl*>>(Context::get_nthreads(), Context::parallel_mode);

    // Usually, Parmap::apply() executes the provided function on all elements of the array.
    // Here, the executed function does not return the control to the parmap before all the array is processed:
    //   - suspend() should switch back to the worker_context (either maestro or one of its minions) to return
    //     the control to the parmap. Instead, it uses parmap_->next() to steal another work, and does it directly.
    //     It only yields back to worker_context when the work array is exhausted.
    //   - So, resume() is only launched from the parmap for the first job of each minion.
    parmap_->apply(
        [](const actor::ActorImpl* actor) {
          auto* context = static_cast<SwappedContext*>(actor->context_.get());
          context->resume();
        },
        actors_list);
  } else { // sequential execution
    if (actors_list.empty())
      return;

    /* maestro is already saved in the first slot of workers_context_ */
    const actor::ActorImpl* first_actor = engine->get_first_actor_to_run();
    process_index_          = 1;
    /* execute the first actor; it will chain to the others when using suspend() */
    static_cast<SwappedContext*>(first_actor->context_.get())->resume();
  }
}

/** Maestro wants to yield back to a given actor, so awake it on the current thread
 *
 * In parallel, it is only applied to the N first elements of the parmap array,
 * where N is the amount of worker threads in the parmap.
 * See SwappedContextFactory::run_all for details.
 */
void SwappedContext::resume()
{
  auto* old = static_cast<SwappedContext*>(self());
  if (is_parallel()) {
    // Save my current soul (either maestro, or one of the minions) in a thread-specific area
    worker_context_ = old;
  }
  sthread_enable();
  // Switch my soul and the actor's one
  Context::set_current(this);
  old->swap_into(this);
  // No body runs that soul anymore at this point, but it is stored in a safe place.
  // When the executed actor will do a blocking action, ActorImpl::yield() will call suspend(), below.
}

/** The actor wants to yield back to maestro, because it is blocked in a simcall (i.e., in ActorImpl::yield())
 *
 * Actually, it does not really yield back to maestro, but directly into the next executable actor.
 *
 * This makes the parmap::apply awkward (see SwappedContextFactory::run_all()) because it only apply regularly
 * on the few first elements of the array, but it saves a lot of context switches back to maestro,
 * and directly forth to the next executable actor.
 */
void SwappedContext::suspend()
{
  SwappedContext* next_context;
  if (is_parallel()) {
    // Get some more work to directly swap into the next executable actor instead of yielding back to the parmap
    boost::optional<actor::ActorImpl*> next_work = factory_.parmap_->next();
    if (next_work) {
      // There is a next soul to embody (ie, another executable actor)
      XBT_DEBUG("Run next process");
      next_context = static_cast<SwappedContext*>(next_work.get()->context_.get());
    } else {
      // All actors were run, go back to the parmap context
      XBT_DEBUG("No more actors to run");
      // worker_context_ is my own soul, stored in thread_local when starting the scheduling round
      next_context = worker_context_;
      // When given that soul, the body will wait for the next scheduling round
    }
  } else { // sequential execution
    const auto* engine = EngineImpl::get_instance();
    /* determine the next context */
    unsigned long int i = factory_.process_index_;
    factory_.process_index_++;

    if (i < engine->get_actor_to_run_count()) {
      /* Actually swap into the next actor directly without transiting to maestro */
      XBT_DEBUG("Run next actor");
      sthread_enable();
      next_context = static_cast<SwappedContext*>(engine->get_actor_to_run_at(i)->context_.get());
    } else {
      /* all processes were run, actually return to maestro */
      XBT_DEBUG("No more actors to run");
      sthread_disable();
      next_context = factory_.maestro_context_;
    }
  }
  Context::set_current(next_context);
  this->swap_into(next_context);
}

} // namespace simgrid::kernel::context
