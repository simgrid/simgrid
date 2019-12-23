/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file UContext.cpp Context switching with ucontexts from System V        */

#include "context_private.hpp"

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/mc_ignore.hpp"
#include "src/simix/smx_private.hpp"

#include "ContextUnix.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

/** Up to two integers may be needed to store a pointer on the system we target */
constexpr int CTX_ADDR_LEN = 2;

static_assert(sizeof(simgrid::kernel::context::UContext*) <= CTX_ADDR_LEN * sizeof(int),
              "Ucontexts are not supported on this arch yet. Please increase CTX_ADDR_LEN.");

/* Make sure that this symbol is easy to recognize by name, even on exotic platforms */
extern "C" {
static void smx_ctx_wrapper(int i1, int i2);
}

// The name of this function is currently hardcoded in MC (as string).
// Do not change it without fixing those references as well.
static void smx_ctx_wrapper(int i1, int i2)
{
  // Rebuild the Context* pointer from the integers:
  int ctx_addr[CTX_ADDR_LEN] = {i1, i2};
  simgrid::kernel::context::UContext* context;
  memcpy(&context, ctx_addr, sizeof context);

  ASAN_FINISH_SWITCH(nullptr, &context->asan_ctx_->asan_stack_, &context->asan_ctx_->asan_stack_size_);
  try {
    (*context)();
    context->Context::stop();
  } catch (simgrid::ForcefulKillException const&) {
    XBT_DEBUG("Caught a ForcefulKillException");
  } catch (simgrid::Exception const& e) {
    XBT_INFO("Actor killed by an uncaught exception %s", simgrid::xbt::demangle(typeid(e).name()).get());
    throw;
  }
  ASAN_ONLY(context->asan_stop_ = true);
  context->suspend();
}

namespace simgrid {
namespace kernel {
namespace context {

// UContextFactory
UContext* UContextFactory::create_context(std::function<void()>&& code, actor::ActorImpl* actor)
{
  return new_context<UContext>(std::move(code), actor, this);
}


// UContext

UContext::UContext(std::function<void()>&& code, actor::ActorImpl* actor, SwappedContextFactory* factory)
    : SwappedContext(std::move(code), actor, factory)
{
  /* if the user provided a function for the actor then use it. If not, nothing to do for maestro. */
  if (has_code()) {
    getcontext(&this->uc_);
    this->uc_.uc_link = nullptr;
    this->uc_.uc_stack.ss_sp   = sg_makecontext_stack_addr(get_stack());
    this->uc_.uc_stack.ss_size = sg_makecontext_stack_size(smx_context_stack_size);
    // Makecontext expects integer arguments; we want to pass a pointer.
    // This context address is decomposed into a series of integers, which are passed as arguments to makecontext.

    int ctx_addr[CTX_ADDR_LEN]{};
    UContext* arg = this;
    memcpy(ctx_addr, &arg, sizeof this);
    makecontext(&this->uc_, (void (*)())smx_ctx_wrapper, 2, ctx_addr[0], ctx_addr[1]);

#if SIMGRID_HAVE_MC
    if (MC_is_active()) {
      MC_register_stack_area(get_stack(), &(this->uc_), smx_context_stack_size);
    }
#endif
  }
}

void UContext::swap_into(SwappedContext* to_)
{
  const UContext* to = static_cast<UContext*>(to_);
  ASAN_ONLY(void* fake_stack = nullptr);
  ASAN_ONLY(to_->asan_ctx_ = this);
  ASAN_START_SWITCH(this->asan_stop_ ? nullptr : &fake_stack, to_->asan_stack_, to_->asan_stack_size_);
  swapcontext(&this->uc_, &to->uc_);
  ASAN_FINISH_SWITCH(fake_stack, &this->asan_ctx_->asan_stack_, &this->asan_ctx_->asan_stack_size_);
}


XBT_PRIVATE ContextFactory* sysv_factory()
{
  XBT_VERB("Activating SYSV context factory");
  return new UContextFactory();
}
} // namespace context
} // namespace kernel
} // namespace simgrid
