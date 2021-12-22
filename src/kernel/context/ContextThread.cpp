/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/context/ContextThread.hpp"

#include "simgrid/Exception.hpp"
#include "src/internal_config.h" /* loads context system definitions */
#include "src/kernel/EngineImpl.hpp"
#include "xbt/function_types.h"
#include "xbt/xbt_modinter.h" /* prototype of os thread module's init/exit in XBT */

#include <boost/core/demangle.hpp>
#include <functional>
#include <typeinfo>
#include <utility>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_context);

namespace simgrid {
namespace kernel {
namespace context {

// ThreadContextFactory

ThreadContextFactory::ThreadContextFactory() : ContextFactory()
{
  if (stack_size != 8 * 1024 * 1024)
    XBT_INFO("Stack size modifications are ignored by thread factory.");
  if (is_parallel())
    ParallelThreadContext::initialize();
}

ThreadContextFactory::~ThreadContextFactory()
{
  if (is_parallel())
    ParallelThreadContext::finalize();
}

ThreadContext* ThreadContextFactory::create_context(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro)
{
  if (is_parallel())
    return this->new_context<ParallelThreadContext>(std::move(code), actor, maestro);
  else
    return this->new_context<SerialThreadContext>(std::move(code), actor, maestro);
}

void ThreadContextFactory::run_all()
{
  if (is_parallel()) {
    // Parallel execution
    ParallelThreadContext::run_all();
  } else {
    // Serial execution
    SerialThreadContext::run_all();
  }
}

// ThreadContext

ThreadContext::ThreadContext(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro)
    : AttachContext(std::move(code), actor, maestro)
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
  install_sigsegv_stack(nullptr, true);
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
    XBT_INFO("Actor killed by an uncaught exception %s", boost::core::demangle(typeid(e).name()).c_str());
    throw;
  }
  // Signal to the caller (normally the maestro) that we have finished:
  context->yield();

#ifndef WIN32
  install_sigsegv_stack(nullptr, false);
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
  auto* maestro = static_cast<ThreadContext*>(EngineImpl::get_instance()->get_maestro()->context_.get());
  maestro->begin_.release();
  xbt_assert(not this->is_maestro());
  this->start();
}

void ThreadContext::attach_stop()
{
  xbt_assert(not this->is_maestro());
  this->yield();

  auto* maestro = static_cast<ThreadContext*>(EngineImpl::get_instance()->get_maestro()->context_.get());
  maestro->end_.acquire();

  Context::set_current(nullptr);
}

// SerialThreadContext

void SerialThreadContext::run_all()
{
  const auto& to_run = EngineImpl::get_instance()->get_actors_to_run();
  for (smx_actor_t const& actor : to_run) {
    XBT_DEBUG("Handling %p", actor);
    auto* context = static_cast<ThreadContext*>(actor->context_.get());
    context->release();
    context->wait();
  }
}

// ParallelThreadContext

xbt::OsSemaphore* ParallelThreadContext::thread_sem_ = nullptr;

void ParallelThreadContext::initialize()
{
  thread_sem_ = new xbt::OsSemaphore(get_nthreads());
}

void ParallelThreadContext::finalize()
{
  delete thread_sem_;
  thread_sem_ = nullptr;
}

void ParallelThreadContext::run_all()
{
  const auto& to_release = EngineImpl::get_instance()->get_actors_to_run();
  for (smx_actor_t const& actor : to_release)
    static_cast<ThreadContext*>(actor->context_.get())->release();
  const auto& to_wait = EngineImpl::get_instance()->get_actors_to_run();
  for (smx_actor_t const& actor : to_wait)
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
