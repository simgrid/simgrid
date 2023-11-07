/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc.h"

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/sthread/sthread.h"

#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_context, kernel, "Context switching mechanism");

namespace simgrid::kernel::context {

void Context::set_nthreads(int nb_threads)
{
  if (nb_threads <= 0) {
    nb_threads = std::thread::hardware_concurrency();
    XBT_INFO("Auto-setting contexts/nthreads to %d", nb_threads);
  }
  Context::parallel_contexts = nb_threads;
}

ContextFactory::~ContextFactory() = default;

e_xbt_parmap_mode_t Context::parallel_mode = XBT_PARMAP_DEFAULT;
int Context::parallel_contexts             = 1;
unsigned Context::stack_size;
unsigned Context::guard_size;
thread_local Context* Context::current_context_ = nullptr;

/* Install or disable alternate signal stack, for SIGSEGV handler. */
int Context::install_sigsegv_stack(bool enable)
{
  static std::vector<unsigned char> sigsegv_stack(SIGSTKSZ);
  stack_t stack;
  stack.ss_sp    = sigsegv_stack.data();
  stack.ss_size  = sigsegv_stack.size();
  stack.ss_flags = enable ? 0 : SS_DISABLE;
  return sigaltstack(&stack, nullptr);
}

Context* Context::self()
{
  return current_context_;
}
void Context::set_current(Context* self)
{
  current_context_ = self;
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
  sthread_disable();
  throw ForcefulKillException(); // clean RAII variables with the dedicated exception
}
AttachContext::~AttachContext() = default;

} // namespace simgrid::kernel::context
