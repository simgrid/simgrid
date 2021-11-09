/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file UContext.cpp Context switching with ucontexts from System V        */

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/mc_ignore.hpp"

#include "ContextUnix.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_context);

/** Up to two integers may be needed to store a pointer on the system we target */
constexpr int CTX_ADDR_LEN = 2;

static_assert(sizeof(simgrid::kernel::context::UContext*) <= CTX_ADDR_LEN * sizeof(int),
              "Ucontexts are not supported on this arch yet. Please increase CTX_ADDR_LEN.");

// This function is called by makecontext(3): use extern "C" to have C language linkage for its type
extern "C" {
XBT_ATTRIB_NORETURN static void sysv_ctx_wrapper(int i1, int i2)
{
  // Rebuild the Context* pointer from the integers:
  int ctx_addr[CTX_ADDR_LEN] = {i1, i2};
  simgrid::kernel::context::UContext* context;
  memcpy(&context, ctx_addr, sizeof context);
  smx_ctx_wrapper(context);
}
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
  XBT_VERB("Creating a context of stack %uMb", actor->get_stacksize() / 1024 / 1024);
  /* if the user provided a function for the actor then use it. If not, nothing to do for maestro. */
  if (has_code()) {
    getcontext(&this->uc_);
    this->uc_.uc_link = nullptr;
    this->uc_.uc_stack.ss_sp   = sg_makecontext_stack_addr(get_stack());
    this->uc_.uc_stack.ss_size = sg_makecontext_stack_size(actor->get_stacksize());
    // Makecontext expects integer arguments; we want to pass a pointer.
    // This context address is decomposed into a series of integers, which are passed as arguments to makecontext.

    int ctx_addr[CTX_ADDR_LEN]{};
    UContext* arg = this;
    memcpy(ctx_addr, &arg, sizeof arg);
    makecontext(&this->uc_, (void (*)())sysv_ctx_wrapper, 2, ctx_addr[0], ctx_addr[1]);

#if SIMGRID_HAVE_MC
    if (MC_is_active()) {
      MC_register_stack_area(get_stack(), &(this->uc_), stack_size);
    }
#endif
  }
}

void UContext::swap_into_for_real(SwappedContext* to_)
{
  const UContext* to = static_cast<UContext*>(to_);
  swapcontext(&this->uc_, &to->uc_);
}


XBT_PRIVATE ContextFactory* sysv_factory()
{
  XBT_VERB("Activating SYSV context factory");
  return new UContextFactory();
}
} // namespace context
} // namespace kernel
} // namespace simgrid
