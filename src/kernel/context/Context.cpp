/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"

#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);


namespace simgrid {
namespace kernel {
namespace context {

ContextFactoryInitializer factory_initializer = nullptr;

ContextFactory::~ContextFactory() = default;

static thread_local Context* smx_current_context = nullptr;
Context* Context::self()
{
  return smx_current_context;
}
void Context::set_current(Context* self)
{
  smx_current_context = self;
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

Context::Context(std::function<void()>&& code, actor::ActorImpl* actor) : code_(std::move(code)), actor_(actor)
{
  /* If no function was provided, this is the context for maestro
   * and we should set it as the current context */
  if (not has_code())
    set_current(this);
}

Context::~Context()
{
  if (self() == this)
    set_current(nullptr);
}

void Context::stop()
{
  this->actor_->cleanup();
}

AttachContext::~AttachContext() = default;

} // namespace context
} // namespace kernel
} // namespace simgrid
