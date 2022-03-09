/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"

#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/surf/surf_interface.hpp"

#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_context, kernel, "Context switching mechanism");

namespace simgrid {
namespace kernel {
namespace context {

ContextFactoryInitializer factory_initializer = nullptr;
static e_xbt_parmap_mode_t parallel_synchronization_mode = XBT_PARMAP_DEFAULT;
static int parallel_contexts                             = 1;
unsigned stack_size;
unsigned guard_size;

/** @brief Returns whether some parallel threads are used for the user contexts. */
bool is_parallel()
{
  return parallel_contexts > 1;
}

/**
 * @brief Returns the number of parallel threads used for the user contexts.
 * @return the number of threads (1 means no parallelism)
 */
int get_nthreads()
{
  return parallel_contexts;
}

/**
 * @brief Sets the number of parallel threads to use  for the user contexts.
 *
 * This function should be called before initializing SIMIX.
 * A value of 1 means no parallelism (1 thread only).
 * If the value is greater than 1, the thread support must be enabled.
 *
 * @param nb_threads the number of threads to use
 */
void set_nthreads(int nb_threads)
{
  if (nb_threads <= 0) {
    nb_threads = std::thread::hardware_concurrency();
    XBT_INFO("Auto-setting contexts/nthreads to %d", nb_threads);
  }
  parallel_contexts = nb_threads;
}

/**
 * @brief Sets the synchronization mode to use when actors are run in parallel.
 * @param mode how to synchronize threads if actors are run in parallel
 */
void set_parallel_mode(e_xbt_parmap_mode_t mode)
{
  parallel_synchronization_mode = mode;
}

/**
 * @brief Returns the synchronization mode used when actors are run in parallel.
 * @return how threads are synchronized if actors are run in parallel
 */
e_xbt_parmap_mode_t get_parallel_mode()
{
  return parallel_synchronization_mode;
}

ContextFactory::~ContextFactory() = default;

thread_local Context* Context::current_context_ = nullptr;

#ifndef WIN32
/* Install or disable alternate signal stack, for SIGSEGV handler. */
int Context::install_sigsegv_stack(stack_t* old_stack, bool enable)
{
  static std::vector<unsigned char> sigsegv_stack(SIGSTKSZ);
  stack_t stack;
  stack.ss_sp    = sigsegv_stack.data();
  stack.ss_size  = sigsegv_stack.size();
  stack.ss_flags = enable ? 0 : SS_DISABLE;
  return sigaltstack(&stack, old_stack);
}
#endif

Context* Context::self()
{
  return current_context_;
}
void Context::set_current(Context* self)
{
  current_context_ = self;
}

void Context::declare_context(std::size_t size)
{
#if SIMGRID_HAVE_MC
  /* Store the address of the stack in heap to compare it apart of heap comparison */
  if(MC_is_active())
    MC_ignore_heap(this, size);
#endif
}

Context* ContextFactory::attach(actor::ActorImpl*)
{
  xbt_die("Cannot attach with this ContextFactory.\n"
    "Try using --cfg=contexts/factory:thread instead.\n");
}

Context* ContextFactory::create_maestro(std::function<void()>&&, actor::ActorImpl*)
{
  xbt_die("Cannot create_maestro with this ContextFactory.\n"
    "Try using --cfg=contexts/factory:thread instead.\n");
}

Context::Context(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro)
    : code_(std::move(code)), actor_(actor), is_maestro_(maestro)
{
  /* If we are creating maestro, we should set it as the current context */
  if (maestro)
    set_current(this);
}

Context::~Context()
{
  if (self() == this)
    set_current(nullptr);
}

void Context::stop()
{
  this->actor_->cleanup_from_self();
}

void Context::set_wannadie(bool value)
{
  XBT_DEBUG("Actor %s gonna die.", actor_->get_cname());
  iwannadie_ = value;
}
AttachContext::~AttachContext() = default;

} // namespace context
} // namespace kernel
} // namespace simgrid
