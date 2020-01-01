/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

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
  ~BoostContext() override;

  void swap_into(SwappedContext* to) override;

private:
#if BOOST_VERSION < 105600
  boost::context::fcontext_t* fc_ = nullptr;
  typedef intptr_t arg_type;
#elif BOOST_VERSION < 106100
  boost::context::fcontext_t fc_;
  typedef intptr_t arg_type;
#else
  boost::context::detail::fcontext_t fc_;
  typedef boost::context::detail::transfer_t arg_type;
#endif

  static void wrapper(arg_type arg);
};

class BoostContextFactory : public SwappedContextFactory {
public:
  BoostContext* create_context(std::function<void()>&& code, actor::ActorImpl* actor) override;
};
} // namespace context
} // namespace kernel
} // namespace simgrid

#endif
