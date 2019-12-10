/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"
#include "smx_private.hpp"
#include "src/surf/xml/platf_private.hpp" // FIXME: KILLME. There must be a better way than mimicking XML here
#include <simgrid/engine.h>
#include <simgrid/s4u/Engine.hpp>

#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_deployment, simix, "Logging specific to SIMIX (deployment)");

void SIMIX_init_application() // XBT_DEPRECATED_v329
{
  sg_platf_exit();
  sg_platf_init();
}

void SIMIX_launch_application(const std::string& file) // XBT_DEPRECATED_v329
{
  simgrid_load_deployment(file.c_str());
}
void SIMIX_function_register(const std::string& name, xbt_main_func_t code) // XBT_DEPRECATED_v329
{
  simgrid::s4u::Engine::get_instance()->register_function(name, code);
}
void SIMIX_function_register(const std::string& name, void (*code)(std::vector<std::string>)) // XBT_DEPRECATED_v329
{
  simgrid::s4u::Engine::get_instance()->register_function(name, code);
}
void SIMIX_function_register_default(xbt_main_func_t code) // XBT_DEPRECATED_v329
{
  simgrid::s4u::Engine::get_instance()->register_default(code);
}

/**
 * @brief Gets a #smx_actor_t code from the global table.
 *
 * Gets a code function from the global table. Returns nullptr if there are no function registered with the name.
 * This table is then used by #SIMIX_launch_application.
 * @param name the reference name of the function.
 * @return The #smx_actor_t or nullptr.
 */
simgrid::simix::ActorCodeFactory& SIMIX_get_actor_code_factory(const std::string& name)
{
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_get_actor_code_factory.");

  auto i = simix_global->registered_functions.find(name);
  if (i == simix_global->registered_functions.end())
    return simix_global->default_function;
  else
    return i->second;
}

/** @brief Bypass the parser, get arguments, and set function to each process */

void SIMIX_process_set_function(const char* process_host, const char* process_function, xbt_dynar_t arguments,
                                double process_start_time, double process_kill_time) // XBT_ATTRIB_DEPRECATED_v329
{
  simgrid::kernel::routing::ActorCreationArgs actor;

  sg_host_t host = sg_host_by_name(process_host);
  if (not host)
    throw std::invalid_argument(simgrid::xbt::string_printf("Host '%s' unknown", process_host));
  actor.host = process_host;
  actor.args.push_back(process_function);
  /* add arguments */
  unsigned int i;
  char *arg;
  xbt_dynar_foreach(arguments, i, arg) {
    actor.args.push_back(arg);
  }

  // Check we know how to handle this function name:
  simgrid::simix::ActorCodeFactory& parse_code = SIMIX_get_actor_code_factory(process_function);
  xbt_assert(parse_code, "Function '%s' unknown", process_function);

  actor.function           = process_function;
  actor.host               = process_host;
  actor.kill_time          = process_kill_time;
  actor.start_time         = process_start_time;
  actor.restart_on_failure = false;
  sg_platf_new_actor(&actor);
}

namespace simgrid {
namespace simix {

void register_function(const std::string& name, const ActorCodeFactory& factory)
{
  simix_global->registered_functions[name] = factory;
}

}
}
