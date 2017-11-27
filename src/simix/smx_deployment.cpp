/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <vector>

#include "simgrid/s4u/Host.hpp"
#include "smx_private.hpp"
#include "src/surf/xml/platf_private.hpp" // FIXME: KILLME. There must be a better way than mimicking XML here
#include <xbt/ex.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_deployment, simix, "Logging specific to SIMIX (deployment)");

extern int surf_parse_lineno;

void SIMIX_init_application()
{
  sg_platf_exit();
  sg_platf_init();
}

/**
 * \brief An application deployer.
 *
 * Creates the process described in \a file.
 * \param file a filename of a xml description of the application. This file
 * follows this DTD :
 *
 *     \include surfxml.dtd
 *
 * Here is a small example of such a platform
 *
 *     \include small_deployment.xml
 *
 */
void SIMIX_launch_application(const char *file)
{
  XBT_ATTRIB_UNUSED int parse_status;
  xbt_assert(simix_global, "SIMIX_global_init has to be called before SIMIX_launch_application.");

  SIMIX_init_application();

  surf_parse_open(file);
  try {
    parse_status = surf_parse();
    surf_parse_close();
    xbt_assert(not parse_status, "Parse error at %s:%d", file, surf_parse_lineno);
  }
  catch (xbt_ex& e) {
    XBT_ERROR("Unrecoverable error at %s:%d. The full exception stack follows, in case it helps you to diagnose the problem.",
        file, surf_parse_lineno);
    throw;
  }
}

// Wrap a main() function into a ActorCodeFactory:
static simgrid::simix::ActorCodeFactory toActorCodeFactory(xbt_main_func_t code)
{
  return [code](std::vector<std::string> args) {
    return simgrid::xbt::wrapMain(code, std::move(args));
  };
}

/**
 * \brief Registers a #xbt_main_func_t code in a global table.
 *
 * Registers a code function in a global table.
 * This table is then used by #SIMIX_launch_application.
 * \param name the reference name of the function.
 * \param code the function
 */
void SIMIX_function_register(const char *name, xbt_main_func_t code)
{
  xbt_assert(simix_global,
    "SIMIX_global_init has to be called before SIMIX_function_register.");
  simix_global->registered_functions[name] = toActorCodeFactory(code);
}

/**
 * \brief Registers a #xbt_main_func_t code as default value.
 *
 * Registers a code function as being the default value. This function will get used by SIMIX_launch_application() when there is no registered function of the requested name in.
 * \param code the function
 */
void SIMIX_function_register_default(xbt_main_func_t code)
{
  xbt_assert(simix_global, "SIMIX_global_init has to be called before SIMIX_function_register.");
  simix_global->default_function = toActorCodeFactory(code);
}

/**
 * \brief Gets a #smx_actor_t code from the global table.
 *
 * Gets a code function from the global table. Returns nullptr if there are no function registered with the name.
 * This table is then used by #SIMIX_launch_application.
 * \param name the reference name of the function.
 * \return The #smx_actor_t or nullptr.
 */
simgrid::simix::ActorCodeFactory& SIMIX_get_actor_code_factory(const char *name)
{
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_get_actor_code_factory.");

  auto i = simix_global->registered_functions.find(name);
  if (i == simix_global->registered_functions.end())
    return simix_global->default_function;
  else
    return i->second;
}

/**
 * \brief Bypass the parser, get arguments, and set function to each process
 */

void SIMIX_process_set_function(const char* process_host, const char* process_function, xbt_dynar_t arguments,
                                double process_start_time, double process_kill_time)
{
  s_sg_platf_process_cbarg_t process;

  sg_host_t host = sg_host_by_name(process_host);
  if (not host)
    THROWF(arg_error, 0, "Host '%s' unknown", process_host);
  process.host = process_host;
  process.args.push_back(process_function);
  /* add arguments */
  unsigned int i;
  char *arg;
  xbt_dynar_foreach(arguments, i, arg) {
    process.args.push_back(arg);
  }

  // Check we know how to handle this function name:
  simgrid::simix::ActorCodeFactory& parse_code = SIMIX_get_actor_code_factory(process_function);
  xbt_assert(parse_code, "Function '%s' unknown", process_function);

  process.function = process_function;
  process.host = process_host;
  process.kill_time = process_kill_time;
  process.start_time = process_start_time;
  process.on_failure = SURF_ACTOR_ON_FAILURE_DIE;
  sg_platf_new_process(&process);
}

namespace simgrid {
namespace simix {

void registerFunction(const char* name, ActorCodeFactory factory)
{
  simix_global->registered_functions[name] = std::move(factory);
}

}
}
