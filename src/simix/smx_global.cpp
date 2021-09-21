/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/EngineImpl.hpp"

XBT_LOG_NEW_CATEGORY(simix, "All SIMIX categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_kernel, simix, "Logging specific to SIMIX (kernel)");

/**
 * @ingroup SIMIX_API
 * @brief A clock (in second).
 *
 * @return Return the clock.
 */
double SIMIX_get_clock() // XBT_ATTRIB_DEPRECATED_v332
{
  return simgrid::s4u::Engine::get_clock();
}

void SIMIX_run() // XBT_ATTRIB_DEPRECATED_v332
{
  simgrid::kernel::EngineImpl::get_instance()->run();
}

int SIMIX_is_maestro()
{
  const simgrid::kernel::actor::ActorImpl* self = SIMIX_process_self();
  return self == nullptr || simgrid::kernel::EngineImpl::get_instance()->is_maestro(self);
}
