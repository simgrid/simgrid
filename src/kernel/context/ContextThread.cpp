/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/context/ContextThread.hpp"

#include "simgrid/Exception.hpp"
#include "src/internal_config.h" /* loads context system definitions */
#include "src/simix/smx_private.hpp"
#include "src/xbt_modinter.h" /* prototype of os thread module's init/exit in XBT */
#include "xbt/function_types.h"

#include <functional>
#include <utility>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

namespace simgrid {
namespace kernel {
namespace context {

// ThreadContextFactory

ThreadContextFactory::ThreadContextFactory() : ContextFactory()
{
  if (SIMIX_context_is_parallel())
    ParallelThreadContext::initialize();
}

ThreadContextFactory::~ThreadContextFactory()
{
  if (SIMIX_context_is_parallel())
    ParallelThreadContext::finalize();
}

ThreadContext* ThreadContextFactory::create_context(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro)
{
  if (SIMIX_context_is_parallel())
    return this->new_context<ParallelThreadContext>(std::move(code), actor, maestro);
  else
    return this->new_context<SerialThreadContext>(std::move(code), actor, maestro);
}

void ThreadContextFactory::run_all()
{
  if (SIMIX_context_is_parallel()) {
    // Parallel execution
    ParallelThreadContext::run_all();
  } else {
    // Serial execution
    SerialThreadContext::run_all();
  }
}

// ThreadContext

ThreadContext::ThreadContext(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro)
    : AttachContext(std::move(code), actor), is_maestro_(maestro)
{
  /* If the user provided a function for the actor then use it */
  if (has_code()) {
    /* create and start the actor */
    this->thread_ = new std::thread(ThreadContext::wrapper, this);
    /* wait the starting of the newly created actor */
    this->end_.acquire();
  }

  /* Otherwise, we attach to the current thread */
  else {
    Context::set_current(this);
  }
}

ThreadContext::~ThreadContext()
{
  if (this->thread_) { /* Maestro don't have any thread */
    thread_->join();
    delete thread_;
  }
}

void ThreadContext::wrapper(ThreadContext* context)
{
  Context::set_current(context);

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
    if (not context->is_maestro()) { // Just in case somebody detached maestro
      context->Context::stop();
      context->stop_hook();
    }
  } catch (ForcefulKillException const&) {
    XBT_DEBUG("Caught a ForcefulKillException in Thread::wrapper");
    xbt_assert(not context->is_maestro(), "Maestro shall not receive ForcefulKillExceptions, even when detached.");
  } catch (simgrid::Exception const& e) {
    XBT_INFO("Actor killed by an uncaught exception %s", simgrid::xbt::demangle(typeid(e).name()).get());
    throw;
  }
  // Signal to the caller (normally the maestro) that we have finished:
  context->yield();

#ifndef WIN32
  stack.ss_flags = SS_DISABLE;
  sigaltstack(&stack, nullptr);
#endif
  XBT_DEBUG("Terminating");
  Context::set_current(nullptr);
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
  stop_hook();
  throw ForcefulKillException();
}

void ThreadContext::suspend()
{
  this->yield();
  this->start();
}

void ThreadContext::attach_start()
{
  // We're breaking the layers here by depending on the upper layer:
  ThreadContext* maestro = static_cast<ThreadContext*>(simix_global->maestro_->context_.get());
  maestro->begin_.release();
  xbt_assert(not this->is_maestro());
  this->start();
}

void ThreadContext::attach_stop()
{
  xbt_assert(not this->is_maestro());
  this->yield();

  ThreadContext* maestro = static_cast<ThreadContext*>(simix_global->maestro_->context_.get());
  maestro->end_.acquire();

  Context::set_current(nullptr);
}

// SerialThreadContext

void SerialThreadContext::run_all()
{
  for (smx_actor_t const& actor : simix_global->actors_to_run) {
    XBT_DEBUG("Handling %p", actor);
    ThreadContext* context = static_cast<ThreadContext*>(actor->context_.get());
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
  for (smx_actor_t const& actor : simix_global->actors_to_run)
    static_cast<ThreadContext*>(actor->context_.get())->release();
  for (smx_actor_t const& actor : simix_global->actors_to_run)
    static_cast<ThreadContext*>(actor->context_.get())->wait();
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
} // namespace context
} // namespace kernel
} // namespace simgrid
