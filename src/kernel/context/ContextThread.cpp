/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/context/ContextThread.hpp"

#include "simgrid/Exception.hpp"
#include "src/internal_config.h" /* loads context system definitions */
#include "src/simix/smx_private.hpp"
#include "src/xbt_modinter.h" /* prototype of os thread module's init/exit in XBT */
#include "xbt/function_types.h"
#include "xbt/xbt_os_thread.h"

#include <functional>
#include <utility>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

namespace simgrid {
namespace kernel {
namespace context {

// ThreadContextFactory

ThreadContextFactory::ThreadContextFactory()
    : ContextFactory("ThreadContextFactory"), parallel_(SIMIX_context_is_parallel())
{
  if (parallel_)
    ParallelThreadContext::initialize();
}

ThreadContextFactory::~ThreadContextFactory()
{
  if (parallel_)
    ParallelThreadContext::finalize();
}

ThreadContext* ThreadContextFactory::create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup,
                                                    smx_actor_t process, bool maestro)
{
  if (parallel_)
    return this->new_context<ParallelThreadContext>(std::move(code), cleanup, process, maestro);
  else
    return this->new_context<SerialThreadContext>(std::move(code), cleanup, process, maestro);
}

void ThreadContextFactory::run_all()
{
  if (parallel_) {
    // Parallel execution
    ParallelThreadContext::run_all();
  } else {
    // Serial execution
    SerialThreadContext::run_all();
  }
}

// ThreadContext

ThreadContext::ThreadContext(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t actor, bool maestro)
    : AttachContext(std::move(code), cleanup, actor), is_maestro_(maestro)
{
  /* If the user provided a function for the process then use it */
  if (has_code()) {
    if (smx_context_stack_size_was_set)
      xbt_os_thread_setstacksize(smx_context_stack_size);
    if (smx_context_guard_size_was_set)
      xbt_os_thread_setguardsize(smx_context_guard_size);

    /* create and start the process */
    this->thread_ = xbt_os_thread_create(ThreadContext::wrapper, this);
    /* wait the starting of the newly created process */
    this->end_.acquire();
  }

  /* Otherwise, we attach to the current thread */
  else {
    SIMIX_context_set_current(this);
  }
}

ThreadContext::~ThreadContext()
{
  if (this->thread_) /* If there is a thread (maestro don't have any), wait for its termination */
    xbt_os_thread_join(this->thread_, nullptr);
}

void *ThreadContext::wrapper(void *param)
{
  ThreadContext* context = static_cast<ThreadContext*>(param);
  SIMIX_context_set_current(context);

#ifndef WIN32
  /* Install alternate signal stack, for SIGSEGV handler. */
  stack_t stack;
  stack.ss_sp = sigsegv_stack;
  stack.ss_size = sizeof sigsegv_stack;
  stack.ss_flags = 0;
  sigaltstack(&stack, nullptr);
#endif
  // Tell the caller (normally the maestro) we are starting, and wait for its green light
  context->end_.release();
  context->start();

  try {
    (*context)();
  } catch (StopRequest const&) {
    XBT_DEBUG("Caught a StopRequest");
    xbt_assert(not context->is_maestro(), "Maestro shall not receive StopRequests, even when detached.");
  } catch (simgrid::Exception const& e) {
    XBT_INFO("Actor killed by an uncatched exception %s", simgrid::xbt::demangle(typeid(e).name()).get());
    throw;
  }
  if (not context->is_maestro()) // Just in case somebody detached maestro
    context->Context::stop();

  // Signal to the caller (normally the maestro) that we have finished:
  context->yield();

#ifndef WIN32
  stack.ss_flags = SS_DISABLE;
  sigaltstack(&stack, nullptr);
#endif
  return nullptr;
}

void ThreadContext::release()
{
  this->begin_.release();
}

void ThreadContext::wait()
{
  this->end_.acquire();
}

void ThreadContext::start()
{
  this->begin_.acquire();
  this->start_hook();
}

void ThreadContext::yield()
{
  this->yield_hook();
  this->end_.release();
}

void ThreadContext::stop()
{
  Context::stop();
  throw StopRequest();
}

void ThreadContext::suspend()
{
  this->yield();
  this->start();
}

void ThreadContext::attach_start()
{
  // We're breaking the layers here by depending on the upper layer:
  ThreadContext* maestro = (ThreadContext*)simix_global->maestro_process->context_;
  maestro->begin_.release();
  xbt_assert(not this->is_maestro());
  this->start();
}

void ThreadContext::attach_stop()
{
  xbt_assert(not this->is_maestro());
  this->yield();

  ThreadContext* maestro = (ThreadContext*)simix_global->maestro_process->context_;
  maestro->end_.acquire();

  SIMIX_context_set_current(nullptr);
}

// SerialThreadContext

void SerialThreadContext::run_all()
{
  for (smx_actor_t const& process : simix_global->process_to_run) {
    XBT_DEBUG("Handling %p", process);
    ThreadContext* context = static_cast<ThreadContext*>(process->context_);
    context->release();
    context->wait();
  }
}

// ParallelThreadContext

xbt::OsSemaphore* ParallelThreadContext::thread_sem_ = nullptr;

void ParallelThreadContext::initialize()
{
  thread_sem_ = new xbt::OsSemaphore(SIMIX_context_get_nthreads());
}

void ParallelThreadContext::finalize()
{
  delete thread_sem_;
  thread_sem_ = nullptr;
}

void ParallelThreadContext::run_all()
{
  for (smx_actor_t const& process : simix_global->process_to_run)
    static_cast<ThreadContext*>(process->context_)->release();
  for (smx_actor_t const& process : simix_global->process_to_run)
    static_cast<ThreadContext*>(process->context_)->wait();
}

void ParallelThreadContext::start_hook()
{
  if (not is_maestro()) /* parallel run */
    thread_sem_->acquire();
}

void ParallelThreadContext::yield_hook()
{
  if (not is_maestro()) /* parallel run */
    thread_sem_->release();
}

XBT_PRIVATE ContextFactory* thread_factory()
{
  XBT_VERB("Activating thread context factory");
  return new ThreadContextFactory();
}
}}} // namespace
