/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <utility>
#include <functional>

#include "src/internal_config.h" /* loads context system definitions */
#include "src/simix/smx_private.hpp"
#include "src/xbt_modinter.h" /* prototype of os thread module's init/exit in XBT */
#include "xbt/function_types.h"
#include "xbt/xbt_os_thread.h"

#include "src/kernel/context/ContextThread.hpp"

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

ThreadContext::ThreadContext(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process,
                             bool maestro)
    : AttachContext(std::move(code), cleanup, process), is_maestro_(maestro)
{
  // We do not need the semaphores when maestro is in main,
  // but creating them anyway simplifies things when maestro is externalized
  this->begin_ = xbt_os_sem_init(0);
  this->end_ = xbt_os_sem_init(0);

  /* If the user provided a function for the process then use it */
  if (has_code()) {
    if (smx_context_stack_size_was_set)
      xbt_os_thread_setstacksize(smx_context_stack_size);
    if (smx_context_guard_size_was_set)
      xbt_os_thread_setguardsize(smx_context_guard_size);

    /* create and start the process */
    /* NOTE: The first argument to xbt_os_thread_create used to be the process *
    * name, but now the name is stored at SIMIX level, so we pass a null  */
    this->thread_ = xbt_os_thread_create(nullptr, ThreadContext::wrapper, this, this);
    /* wait the starting of the newly created process */
    xbt_os_sem_acquire(this->end_);
  }

  /* Otherwise, we attach to the current thread */
  else {
    xbt_os_thread_set_extra_data(this);
  }
}

ThreadContext::~ThreadContext()
{
  if (this->thread_) /* If there is a thread (maestro don't have any), wait for its termination */
    xbt_os_thread_join(this->thread_, nullptr);

  /* destroy the synchronization objects */
  xbt_os_sem_destroy(this->begin_);
  xbt_os_sem_destroy(this->end_);
}

void *ThreadContext::wrapper(void *param)
{
  ThreadContext* context = static_cast<ThreadContext*>(param);

#ifndef WIN32
  /* Install alternate signal stack, for SIGSEGV handler. */
  stack_t stack;
  stack.ss_sp = sigsegv_stack;
  stack.ss_size = sizeof sigsegv_stack;
  stack.ss_flags = 0;
  sigaltstack(&stack, nullptr);
#endif
  // Tell the caller (normally the maestro) we are starting, and wait for its green light
  xbt_os_sem_release(context->end_);
  context->start();

  try {
    (*context)();
    if (not context->isMaestro()) // really?
      context->Context::stop();
  } catch (StopRequest const&) {
    XBT_DEBUG("Caught a StopRequest");
    xbt_assert(not context->isMaestro(), "I'm not supposed to be maestro here.");
  }

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
  xbt_os_sem_release(this->begin_);
}

void ThreadContext::wait()
{
  xbt_os_sem_acquire(this->end_);
}

void ThreadContext::start()
{
  xbt_os_sem_acquire(this->begin_);
  this->start_hook();
}

void ThreadContext::yield()
{
  this->yield_hook();
  xbt_os_sem_release(this->end_);
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
  ThreadContext* maestro = (ThreadContext*) simix_global->maestro_process->context;
  xbt_os_sem_release(maestro->begin_);
  xbt_assert(not this->isMaestro());
  this->start();
}

void ThreadContext::attach_stop()
{
  xbt_assert(not this->isMaestro());
  this->yield();

  ThreadContext* maestro = (ThreadContext*) simix_global->maestro_process->context;
  xbt_os_sem_acquire(maestro->end_);

  xbt_os_thread_set_extra_data(nullptr);
}

// SerialThreadContext

void SerialThreadContext::run_all()
{
  for (smx_actor_t const& process : simix_global->process_to_run) {
    XBT_DEBUG("Handling %p", process);
    ThreadContext* context = static_cast<ThreadContext*>(process->context);
    context->release();
    context->wait();
  }
}

// ParallelThreadContext

xbt_os_sem_t ParallelThreadContext::thread_sem_ = nullptr;

void ParallelThreadContext::initialize()
{
  thread_sem_ = xbt_os_sem_init(SIMIX_context_get_nthreads());
}

void ParallelThreadContext::finalize()
{
  xbt_os_sem_destroy(thread_sem_);
  thread_sem_ = nullptr;
}

void ParallelThreadContext::run_all()
{
  for (smx_actor_t const& process : simix_global->process_to_run)
    static_cast<ThreadContext*>(process->context)->release();
  for (smx_actor_t const& process : simix_global->process_to_run)
    static_cast<ThreadContext*>(process->context)->wait();
}

void ParallelThreadContext::start_hook()
{
  if (not isMaestro()) /* parallel run */
    xbt_os_sem_acquire(thread_sem_);
}

void ParallelThreadContext::yield_hook()
{
  if (not isMaestro()) /* parallel run */
    xbt_os_sem_release(thread_sem_);
}

XBT_PRIVATE ContextFactory* thread_factory()
{
  XBT_VERB("Activating thread context factory");
  return new ThreadContextFactory();
}
}}} // namespace
