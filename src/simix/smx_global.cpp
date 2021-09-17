/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/kernel/Timer.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/smpi/include/smpi_actor.hpp"

#include "simgrid/sg_config.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/xml/platf.hpp"

#include "simgrid/kernel/resource/Model.hpp"

#include <memory>

XBT_LOG_NEW_CATEGORY(simix, "All SIMIX categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_kernel, simix, "Logging specific to SIMIX (kernel)");

std::unique_ptr<simgrid::simix::Global> simix_global;

namespace simgrid {
namespace simix {

xbt_dynar_t simix_global_get_actors_addr()
{
#if SIMGRID_HAVE_MC
  return kernel::EngineImpl::get_instance()->get_actors_vector();
#else
  xbt_die("This function is intended to be used when compiling with MC");
#endif
}
xbt_dynar_t simix_global_get_dead_actors_addr()
{
#if SIMGRID_HAVE_MC
  return kernel::EngineImpl::get_instance()->get_dead_actors_vector();
#else
  xbt_die("This function is intended to be used when compiling with MC");
#endif
}

} // namespace simix
} // namespace simgrid


static simgrid::kernel::actor::ActorCode maestro_code;
void SIMIX_set_maestro(void (*code)(void*), void* data)
{
#ifdef _WIN32
  XBT_INFO("WARNING, SIMIX_set_maestro is believed to not work on windows. Please help us investigating this issue if "
           "you need that feature");
#endif
  maestro_code = std::bind(code, data);
}

void SIMIX_global_init(int* argc, char** argv)
{
  if (simix_global != nullptr)
    return;

  simix_global = std::make_unique<simgrid::simix::Global>();

  SIMIX_context_mod_init();

  // Either create a new context with maestro or create
  // a context object with the current context maestro):
  simgrid::kernel::actor::create_maestro(maestro_code);
}

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
  if (simix_global == nullptr) // SimDag
    return true;
  const simgrid::kernel::actor::ActorImpl* self = SIMIX_process_self();
  return self == nullptr || simgrid::kernel::EngineImpl::get_instance()->is_maestro(self);
}
