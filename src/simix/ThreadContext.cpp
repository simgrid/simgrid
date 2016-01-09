/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <utility>
#include <functional>

#include "xbt/function_types.h"
#include "smx_private.h"
#include "src/portable.h"           /* loads context system definitions */
#include "xbt/swag.h"
#include "xbt/xbt_os_thread.h"
#include "src/xbt_modinter.h"       /* prototype of os thread module's init/exit in XBT */

#include "src/simix/smx_private.hpp"
#include "src/simix/ThreadContext.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

static xbt_os_sem_t smx_ctx_thread_sem = nullptr;

namespace simgrid {
namespace simix {

XBT_PRIVATE ContextFactory* thread_factory()
{
  XBT_VERB("Activating thread context factory");
  return new ThreadContextFactory();
}

ThreadContextFactory::ThreadContextFactory()
  : ContextFactory("ThreadContextFactory")
{
  if (SIMIX_context_is_parallel()) {
    smx_ctx_thread_sem = xbt_os_sem_init(SIMIX_context_get_nthreads());
  } else {
    smx_ctx_thread_sem = nullptr;
  }
}

ThreadContextFactory::~ThreadContextFactory()
{
  if (smx_ctx_thread_sem) {
    xbt_os_sem_destroy(smx_ctx_thread_sem);
    smx_ctx_thread_sem = nullptr;
  }
}

ThreadContext* ThreadContextFactory::create_context(
    std::function<void()> code,
    void_pfn_smxprocess_t cleanup, smx_process_t process)
{
  return this->new_context<ThreadContext>(std::move(code), cleanup, process);
}

ThreadContext::ThreadContext(std::function<void()> code,
    void_pfn_smxprocess_t cleanup, smx_process_t process)
  : Context(std::move(code), cleanup, process)
{
  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if (has_code()) {
    this->begin_ = xbt_os_sem_init(0);
    this->end_ = xbt_os_sem_init(0);
    if (smx_context_stack_size_was_set)
      xbt_os_thread_setstacksize(smx_context_stack_size);
    if (smx_context_guard_size_was_set)
      xbt_os_thread_setguardsize(smx_context_guard_size);

    /* create and start the process */
    /* NOTE: The first argument to xbt_os_thread_create used to be the process *
    * name, but now the name is stored at SIMIX level, so we pass a null  */
    this->thread_ =
      xbt_os_thread_create(NULL, ThreadContext::wrapper, this, this);

    /* wait the starting of the newly created process */
    xbt_os_sem_acquire(this->end_);

  } else {
    xbt_os_thread_set_extra_data(this);
  }
}

void ThreadContextFactory::run_all()
{
  if (smx_ctx_thread_sem == nullptr) {
    // Serial execution
    smx_process_t process;
    unsigned int cursor;
    xbt_dynar_foreach(simix_global->process_to_run, cursor, process) {
      XBT_DEBUG("Handling %p",process);
      ThreadContext* context = static_cast<ThreadContext*>(process->context);
      xbt_os_sem_release(context->begin_);
      xbt_os_sem_acquire(context->end_);
    }
  } else {
    // Parallel execution
    unsigned int index;
    smx_process_t process;
    xbt_dynar_foreach(simix_global->process_to_run, index, process)
      xbt_os_sem_release(static_cast<ThreadContext*>(process->context)->begin_);
    xbt_dynar_foreach(simix_global->process_to_run, index, process)
       xbt_os_sem_acquire(static_cast<ThreadContext*>(process->context)->end_);
  }
}

ThreadContext* ThreadContextFactory::self()
{
  return static_cast<ThreadContext*>(xbt_os_thread_get_extra_data());
}

ThreadContext::~ThreadContext()
{
  /* check if this is the context of maestro (it doesn't have a real thread) */
  if (this->thread_) {
    /* wait about the thread terminason */
    xbt_os_thread_join(this->thread_, nullptr);

    /* destroy the synchronisation objects */
    xbt_os_sem_destroy(this->begin_);
    xbt_os_sem_destroy(this->end_);
  }
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
  /* Tell the maestro we are starting, and wait for its green light */
  xbt_os_sem_release(context->end_);
  xbt_os_sem_acquire(context->begin_);
  if (smx_ctx_thread_sem)       /* parallel run */
    xbt_os_sem_acquire(smx_ctx_thread_sem);

  (*context)();
  context->stop();
  return nullptr;
}

void ThreadContext::stop()
{
  Context::stop();
  if (smx_ctx_thread_sem)
    xbt_os_sem_release(smx_ctx_thread_sem);

  // Signal to the maestro that it has finished:
  xbt_os_sem_release(this->end_);

  xbt_os_thread_exit(NULL);
}

void ThreadContext::suspend()
{
  if (smx_ctx_thread_sem)
    xbt_os_sem_release(smx_ctx_thread_sem);
  xbt_os_sem_release(this->end_);
  xbt_os_sem_acquire(this->begin_);
  if (smx_ctx_thread_sem)
    xbt_os_sem_acquire(smx_ctx_thread_sem);
}

}
}
