/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "ContextBoost.hpp"
#include "simgrid/Exception.hpp"
#include "src/internal_config.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_context);

namespace simgrid {
namespace kernel {
namespace context {

// BoostContextFactory
BoostContext* BoostContextFactory::create_context(std::function<void()>&& code, actor::ActorImpl* actor)
{
  return this->new_context<BoostContext>(std::move(code), actor, this);
}

// BoostContext

BoostContext::BoostContext(std::function<void()>&& code, actor::ActorImpl* actor, SwappedContextFactory* factory)
    : SwappedContext(std::move(code), actor, factory)
{
  XBT_VERB("Creating a context of stack %uMb", actor->get_stacksize() / 1024 / 1024);
  /* if the user provided a function for the process then use it, otherwise it is the context for maestro */
  if (has_code()) {
#if BOOST_VERSION < 106100
    this->fc_ = boost::context::make_fcontext(get_stack_bottom(), actor->get_stacksize(), BoostContext::wrapper);
#else
    this->fc_ =
        boost::context::detail::make_fcontext(get_stack_bottom(), actor->get_stacksize(), BoostContext::wrapper);
#endif
  }
}

void BoostContext::wrapper(BoostContext::arg_type arg)
{
#if BOOST_VERSION < 106100
  BoostContext* context = reinterpret_cast<BoostContext*>(arg);
#else
  BoostContext* context = static_cast<BoostContext**>(arg.data)[1];
  context->verify_previous_context(static_cast<BoostContext**>(arg.data)[0]);
  static_cast<BoostContext**>(arg.data)[0]->fc_ = arg.fctx;
#endif
  smx_ctx_wrapper(context);
}

void BoostContext::swap_into_for_real(SwappedContext* to_)
{
  auto* to = static_cast<BoostContext*>(to_);
#if BOOST_VERSION < 106100
  boost::context::jump_fcontext(&this->fc_, to->fc_, reinterpret_cast<intptr_t>(to));
#else
  BoostContext* ctx[2] = {this, to};
  boost::context::detail::transfer_t arg = boost::context::detail::jump_fcontext(to->fc_, ctx);
  this->verify_previous_context(static_cast<BoostContext**>(arg.data)[0]);
  static_cast<BoostContext**>(arg.data)[0]->fc_ = arg.fctx;
#endif
}

XBT_PRIVATE ContextFactory* boost_factory()
{
  XBT_VERB("Using Boost contexts. Welcome to the 21th century.");
  return new BoostContextFactory();
}
} // namespace context
} // namespace kernel
} // namespace simgrid
