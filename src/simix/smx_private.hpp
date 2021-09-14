/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_PRIVATE_HPP
#define SIMIX_PRIVATE_HPP

#include "simgrid/s4u/Actor.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/context/Context.hpp"

/********************************** Simix Global ******************************/

namespace simgrid {
namespace simix {

class Global {
  kernel::context::ContextFactory* context_factory_ = nullptr;

public:
  kernel::context::ContextFactory* get_context_factory() const { return context_factory_; }
  void set_context_factory(kernel::context::ContextFactory* factory) { context_factory_ = factory; }
  bool has_context_factory() const { return context_factory_ != nullptr; }
  void destroy_context_factory()
  {
    delete context_factory_;
    context_factory_ = nullptr;
  }
};
}
}

XBT_PUBLIC_DATA std::unique_ptr<simgrid::simix::Global> simix_global;

#endif
