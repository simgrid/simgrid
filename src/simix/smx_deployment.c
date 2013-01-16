/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "surf/surfxml_parse.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_deployment, simix,
                                "Logging specific to SIMIX (deployment)");

static xbt_main_func_t parse_code = NULL;
static double start_time = 0.0;
static double kill_time = -1.0;

static int auto_restart = 0;

extern int surf_parse_lineno;

static void parse_process(sg_platf_process_cbarg_t process)
{
  smx_host_t host = SIMIX_host_get_by_name(process->host);
  if (!host)
    THROWF(arg_error, 0, "Host '%s' unknown", process->host);
  parse_code = SIMIX_get_registered_function(process->function);
  xbt_assert(parse_code, "Function '%s' unknown", process->function);

  start_time = process->start_time;
  kill_time  = process->kill_time;
  auto_restart = process->on_failure == SURF_PROCESS_ON_FAILURE_DIE ? 0 : 1;

  smx_process_arg_t arg = NULL;
  smx_process_t process_created = NULL;

  if (start_time > SIMIX_get_clock()) {
    arg = xbt_new0(s_smx_process_arg_t, 1);
    arg->name = (char*)(process->argv)[0];
    arg->code = parse_code;
    arg->data = NULL;
    arg->hostname = host->name;
    arg->argc = process->argc;
    arg->argv = (char**)(process->argv);
    arg->kill_time = kill_time;
    arg->properties = current_property_set;

    XBT_DEBUG("Process %s(%s) will be started at time %f", arg->name,
           arg->hostname, start_time);
    SIMIX_timer_set(start_time, &SIMIX_process_create_from_wrapper, arg);
  } else {                      // start_time <= SIMIX_get_clock()
    XBT_DEBUG("Starting Process %s(%s) right now", process->argv[0], host->name);

    if (simix_global->create_process_function)
      simix_global->create_process_function(&process_created,
                                            (char*)(process->argv)[0],
                                            parse_code,
                                            NULL,
                                            host->name,
                                            kill_time,
                                            process->argc,
                                            (char**)(process->argv),
                                            current_property_set,
                                            auto_restart);
    else
      simcall_process_create(&process_created, (char*)(process->argv)[0], parse_code, NULL, host->name, kill_time, process->argc,
          (char**)process->argv, current_property_set,auto_restart);

    /* verify if process has been created (won't be the case if the host is currently dead, but that's fine) */
    if (!process_created) {
      return;
    }
  }
  current_property_set = NULL;
}

void SIMIX_init_application(void){
  surf_parse_reset_callbacks();
  sg_platf_process_add_cb(parse_process);
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
  xbt_ex_t e;

  _XBT_GNUC_UNUSED int parse_status;
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_launch_application.");

  SIMIX_init_application();

  surf_parse_open(file);
  TRY {
    parse_status = surf_parse();
    surf_parse_close();
    xbt_assert(!parse_status, "Parse error at %s:%d", file,surf_parse_lineno);
  } CATCH(e) {
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
XBT_INLINE void SIMIX_function_register(const char *name,
                                        xbt_main_func_t code)
{
  xbt_assert(simix_global,
              "SIMIX_global_init has to be called before SIMIX_function_register.");

  xbt_dict_set(simix_global->registered_functions, name, code, NULL);
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
  s_sg_platf_process_cbarg_t process;
  memset(&process,0,sizeof(process));

  smx_host_t host = SIMIX_host_get_by_name(process_host);
  if (!host)
    THROWF(arg_error, 0, "Host '%s' unknown", process_host);
  process.host = host->name;

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

  parse_code = SIMIX_get_registered_function(process_function);
  xbt_assert(parse_code, "Function '%s' unknown", process_function);
  process.function = process_function;

  process.host = process_host;
  process.kill_time = process_kill_time;
  process.start_time = process_start_time;
  process.on_failure = SURF_PROCESS_ON_FAILURE_DIE;
  process.properties = NULL;

  sg_platf_new_process(&process);
}
