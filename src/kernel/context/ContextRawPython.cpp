/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */
/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/context/ContextRawPython.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "xbt/log.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_context);

// raw_makecontext and raw_swapcontext are defined in ContextRaw.cpp as
// extern "C" symbols but not declared in ContextRaw.hpp, so forward-declare
// them here.  smx_ctx_wrapper is in ContextSwapped.hpp (included above).
using raw_stack_t          = void*;
using rawctx_entry_point_t = void (*)(simgrid::kernel::context::SwappedContext*);
extern "C" raw_stack_t raw_makecontext(void* malloced_stack, int stack_size,
                                        rawctx_entry_point_t entry_point,
                                        simgrid::kernel::context::SwappedContext* arg);
extern "C" void raw_swapcontext(raw_stack_t* old, raw_stack_t new_context);

namespace simgrid::kernel::context {

void (*RawPythonContext::py_switch_)(PyState*, PyState*) = nullptr;

void RawPythonContext::register_py_switch(void (*fn)(PyState* from, PyState* to))
{
  py_switch_ = fn;
}

RawPythonContext::RawPythonContext(std::function<void()>&& code,
                                    actor::ActorImpl* actor,
                                    SwappedContextFactory* factory)
    : SwappedContext(std::move(code), actor, factory)
{
  if (has_code())
    stack_top_ = raw_makecontext(get_stack(), actor->get_stacksize(), smx_ctx_wrapper, this);
}

void RawPythonContext::swap_into_for_real(SwappedContext* to_)
{
  auto* to = static_cast<RawPythonContext*>(to_);
  if (py_switch_)
    py_switch_(&py_state_, &to->py_state_);
  raw_swapcontext(&stack_top_, to->stack_top_);
}

RawPythonContext* RawPythonContextFactory::create_context(std::function<void()>&& code,
                                                           actor::ActorImpl* actor)
{
  return new_context<RawPythonContext>(std::move(code), actor, this);
}

ContextFactory* raw_python_factory()
{
  XBT_DEBUG("Using RawPythonContextFactory (fast raw contexts + Python interpreter state isolation)");
  return new RawPythonContextFactory();
}

} // namespace simgrid::kernel::context
