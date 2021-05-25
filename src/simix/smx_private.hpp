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
public:

  kernel::context::ContextFactory* context_factory = nullptr;
  kernel::actor::ActorImpl* maestro_ = nullptr;

};
}
}

XBT_PUBLIC_DATA std::unique_ptr<simgrid::simix::Global> simix_global;

XBT_PUBLIC void SIMIX_clean();

#endif
