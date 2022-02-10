/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_BOOST_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_BOOST_CONTEXT_HPP

#include <boost/version.hpp>
#if BOOST_VERSION < 106100
#include <boost/context/fcontext.hpp>
#else
#include <boost/context/detail/fcontext.hpp>
#endif

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include <simgrid/simix.hpp>
#include <xbt/parmap.hpp>

#include "src/internal_config.h"
#include "src/kernel/context/Context.hpp"
#include "src/kernel/context/ContextSwapped.hpp"

namespace simgrid {
namespace kernel {
namespace context {

/** @brief Userspace context switching implementation based on Boost.Context */
class BoostContext : public SwappedContext {
public:
  BoostContext(std::function<void()>&& code, actor::ActorImpl* actor, SwappedContextFactory* factory);

private:
#if BOOST_VERSION < 106100
  boost::context::fcontext_t fc_{};
  using arg_type = intptr_t;
#else
  boost::context::detail::fcontext_t fc_{};
  using arg_type = boost::context::detail::transfer_t;
#endif

  XBT_ATTRIB_NORETURN static void wrapper(arg_type arg);

  void swap_into_for_real(SwappedContext* to) override;
};

class BoostContextFactory : public SwappedContextFactory {
public:
  BoostContext* create_context(std::function<void()>&& code, actor::ActorImpl* actor) override;
};
} // namespace context
} // namespace kernel
} // namespace simgrid

#endif
