/* Copyright (c) 2007, 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "src/surf/xml/platf_private.hpp" // FIXME: KILLME. There must be a better way than mimicking XML here

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_deployment, simix,
                                "Logging specific to SIMIX (deployment)");

extern int surf_parse_lineno;

void SIMIX_init_application(void)
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
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_launch_application.");

  SIMIX_init_application();

  surf_parse_open(file);
  TRY {
    parse_status = surf_parse();
    surf_parse_close();
    xbt_assert(!parse_status, "Parse error at %s:%d", file,surf_parse_lineno);
  }
  CATCH_ANONYMOUS {
    XBT_ERROR("Unrecoverable error at %s:%d. The full exception stack follows, in case it helps you to diagnose the problem.",
        file, surf_parse_lineno);
    RETHROW;
  }
}

/**
 * \brief Registers a #smx_process_code_t code in a global table.
 *
 * Registers a code function in a global table.
 * This table is then used by #SIMIX_launch_application.
 * \param name the reference name of the function.
 * \param code the function
 */
void SIMIX_function_register(const char *name, xbt_main_func_t code)
{
  xbt_assert(simix_global, "SIMIX_global_init has to be called before SIMIX_function_register.");
  xbt_dict_set(simix_global->registered_functions, name, (void*) code, NULL);
}

static xbt_main_func_t default_function = NULL;
/**
 * \brief Registers a #smx_process_code_t code as default value.
 *
 * Registers a code function as being the default value. This function will get used by SIMIX_launch_application() when there is no registered function of the requested name in.
 * \param code the function
 */
void SIMIX_function_register_default(xbt_main_func_t code)
{
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_function_register.");

  default_function = code;
}

/**
 * \brief Gets a #smx_process_t code from the global table.
 *
 * Gets a code function from the global table. Returns NULL if there are no function registered with the name.
 * This table is then used by #SIMIX_launch_application.
 * \param name the reference name of the function.
 * \return The #smx_process_t or NULL.
 */
xbt_main_func_t SIMIX_get_registered_function(const char *name)
{
  xbt_main_func_t res = NULL;
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_get_registered_function.");

  res = (xbt_main_func_t)xbt_dict_get_or_null(simix_global->registered_functions, name);
  return res ? res : default_function;
}


/**
 * \brief Bypass the parser, get arguments, and set function to each process
 */

void SIMIX_process_set_function(const char *process_host,
                                const char *process_function,
                                xbt_dynar_t arguments,
                                double process_start_time,
                                double process_kill_time)
{
  s_sg_platf_process_cbarg_t process = SG_PLATF_PROCESS_INITIALIZER;

  sg_host_t host = sg_host_by_name(process_host);
  if (!host)
    THROWF(arg_error, 0, "Host '%s' unknown", process_host);
  process.host = sg_host_get_name(host);

  process.argc = 1 + xbt_dynar_length(arguments);
  process.argv = (const char**)xbt_new(char *, process.argc + 1);
  process.argv[0] = xbt_strdup(process_function);
  /* add arguments */
  unsigned int i;
  char *arg;
  xbt_dynar_foreach(arguments, i, arg) {
    process.argv[i + 1] = xbt_strdup(arg);
  }
  process.argv[process.argc] = NULL;

  xbt_main_func_t parse_code = SIMIX_get_registered_function(process_function);
  xbt_assert(parse_code, "Function '%s' unknown", process_function);
  process.function = process_function;

  process.host = process_host;
  process.kill_time = process_kill_time;
  process.start_time = process_start_time;

  sg_platf_new_process(&process);
}
