/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file UContext.cpp Context switching with ucontexts from System V        */

#include "context_private.hpp"

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "src/mc/mc_ignore.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.hpp"

#include "ContextUnix.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

/** Many integers are needed to store a pointer
 *
 * Support up to two ints. */
constexpr int CTX_ADDR_LEN = 2;

static_assert(sizeof(simgrid::kernel::context::UContext*) <= CTX_ADDR_LEN * sizeof(int),
              "Ucontexts are not supported on this arch yet");

namespace simgrid {
namespace kernel {
namespace context {

// UContextFactory
Context* UContextFactory::create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process)
{
  return new_context<UContext>(std::move(code), cleanup, process, this);
}


// UContext

UContext::UContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process,
                   SwappedContextFactory* factory)
    : SwappedContext(std::move(code), cleanup_func, process, factory)
{
  /* if the user provided a function for the process then use it, otherwise it is the context for maestro */
  if (has_code()) {
    getcontext(&this->uc_);
    this->uc_.uc_link = nullptr;
    this->uc_.uc_stack.ss_sp   = sg_makecontext_stack_addr(get_stack());
    this->uc_.uc_stack.ss_size = sg_makecontext_stack_size(smx_context_usable_stack_size);
#if PTH_STACKGROWTH == -1
    ASAN_ONLY(this->asan_stack_ = static_cast<char*>(this->stack_) + smx_context_usable_stack_size);
#else
    ASAN_ONLY(this->asan_stack_ = this->stack_);
#endif
    UContext::make_ctx(&this->uc_, UContext::smx_ctx_sysv_wrapper, this);
  } else {
    set_maestro(this); // save maestro for run_all()
  }

#if SIMGRID_HAVE_MC
  if (MC_is_active() && has_code()) {
    MC_register_stack_area(this->stack_, process, &(this->uc_), smx_context_usable_stack_size);
  }
#endif
}

// The name of this function is currently hardcoded in the code (as string).
// Do not change it without fixing those references as well.
void UContext::smx_ctx_sysv_wrapper(int i1, int i2)
{
  // Rebuild the Context* pointer from the integers:
  int ctx_addr[CTX_ADDR_LEN] = {i1, i2};
  simgrid::kernel::context::UContext* context;
  memcpy(&context, ctx_addr, sizeof context);

  ASAN_FINISH_SWITCH(nullptr, &context->asan_ctx_->asan_stack_, &context->asan_ctx_->asan_stack_size_);
  try {
    (*context)();
  } catch (simgrid::kernel::context::Context::StopRequest const&) {
    XBT_DEBUG("Caught a StopRequest");
  } catch (simgrid::Exception const& e) {
    XBT_INFO("Actor killed by an uncatched exception %s", simgrid::xbt::demangle(typeid(e).name()).get());
    throw;
  }
  context->Context::stop();
  ASAN_ONLY(context->asan_stop_ = true);
  context->suspend();
}

/** A better makecontext
 *
 * Makecontext expects integer arguments, we the context variable is decomposed into a serie of integers and each
 * integer is passed as argument to makecontext.
 */
void UContext::make_ctx(ucontext_t* ucp, void (*func)(int, int), UContext* arg)
{
  int ctx_addr[CTX_ADDR_LEN]{};
  memcpy(ctx_addr, &arg, sizeof arg);
  makecontext(ucp, (void (*)())func, 2, ctx_addr[0], ctx_addr[1]);
}

void UContext::swap_into(SwappedContext* to_)
{
  UContext* to = static_cast<UContext*>(to_);
  ASAN_ONLY(void* fake_stack = nullptr);
  ASAN_ONLY(to->asan_ctx_ = this);
  ASAN_START_SWITCH(this->asan_stop_ ? nullptr : &fake_stack, to->asan_stack_, to->asan_stack_size_);
  swapcontext(&this->uc_, &to->uc_);
  ASAN_FINISH_SWITCH(fake_stack, &this->asan_ctx_->asan_stack_, &this->asan_ctx_->asan_stack_size_);
}


XBT_PRIVATE ContextFactory* sysv_factory()
{
  XBT_VERB("Activating SYSV context factory");
  return new UContextFactory();
}
}}} // namespace simgrid::kernel::context
