/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** \ingroup msg_simulation
 * \brief An application deployer.
 *
 * Creates the process described in \a file.
 * \param file a filename of a xml description of the application. This file follows this DTD :
 *
 *     \include simgrid.dtd
 *
 * Here is a small example of such a platform 
 *
 *     \include msg/masterslave/deployment_masterslave.xml
 *
 * Have a look in the directory examples/msg/ to have a bigger example.
 */
void MSG_launch_application(const char *file)
{
  xbt_assert(msg_global, "MSG_init has to be called before MSG_launch_application.");
  SIMIX_launch_application(file);
}

/** \ingroup msg_simulation
 * \brief Registers the main function of a process in a global table.
 *
 * Registers a code function in a global table. 
 * This table is then used by #MSG_launch_application. 
 * \param name the reference name of the function.
 * \param code the function (must have the same prototype than the main function of any C program: int ..(int argc, char *argv[]))
 */
void MSG_function_register(const char *name, xbt_main_func_t code)
{
  SIMIX_function_register(name, code);
}

/** \ingroup msg_simulation
 * \brief Registers a function as the default main function of processes.
 *
 * Registers a code function as being the default value. This function will get used by MSG_launch_application() when there is no registered function of the requested name in.
 * \param code the function (must have the same prototype than the main function of any C program: int ..(int argc, char *argv[]))
 */
void MSG_function_register_default(xbt_main_func_t code)
{
  SIMIX_function_register_default(code);
}

/** \ingroup msg_simulation
 * \brief Retrieves a registered main function
 *
 * Registers a code function in a global table. 
 * This table is then used by #MSG_launch_application. 
 * \param name the reference name of the function.
 */
xbt_main_func_t MSG_get_registered_function(const char *name)
{
  return SIMIX_get_registered_function(name);
}

/**
 * \brief register functions bypassing the parser
 */

void MSG_set_function(const char *host_id, const char *function_name, xbt_dynar_t arguments)
{
  SIMIX_process_set_function(host_id, function_name, arguments, -1, -1);
}
