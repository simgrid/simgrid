/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "surf/surfxml_parse_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_deployment, simix,
                                "Logging specific to SIMIX (deployment)");
static int parse_argc = -1;
static char **parse_argv = NULL;
static xbt_main_func_t parse_code = NULL;
static char *parse_host = NULL;
static double start_time = 0.0;
static double kill_time = -1.0;

static void parse_process_init(void)
{
  parse_host = xbt_strdup(A_surfxml_process_host);
  xbt_assert1(SIMIX_host_get_by_name(parse_host),
              "Host '%s' unknown", parse_host);
  parse_code = SIMIX_get_registered_function(A_surfxml_process_function);
  xbt_assert1(parse_code, "Function '%s' unknown",
              A_surfxml_process_function);
  parse_argc = 0;
  parse_argv = NULL;
  parse_argc++;
  parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
  parse_argv[(parse_argc) - 1] = xbt_strdup(A_surfxml_process_function);
  surf_parse_get_double(&start_time, A_surfxml_process_start_time);
  surf_parse_get_double(&kill_time, A_surfxml_process_kill_time);

  current_property_set = xbt_dict_new();
}

static void parse_argument(void)
{
  parse_argc++;
  parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
  parse_argv[(parse_argc) - 1] = xbt_strdup(A_surfxml_argument_value);
}

static void parse_process_finalize(void)
{
  smx_process_arg_t arg = NULL;
  void *process = NULL;
  if (start_time > SIMIX_get_clock()) {
    arg = xbt_new0(s_smx_process_arg_t, 1);
    arg->name = parse_argv[0];
    arg->code = parse_code;
    arg->data = NULL;
    arg->hostname = parse_host;
    arg->argc = parse_argc;
    arg->argv = parse_argv;
    arg->kill_time = kill_time;
    arg->properties = current_property_set;

    DEBUG3("Process %s(%s) will be started at time %f", arg->name,
           arg->hostname, start_time);
    if (simix_global->create_process_function)
      surf_timer_model->extension_public->set(start_time, (void *)
                                              simix_global->create_process_function,
                                              arg);
    else
      surf_timer_model->extension_public->set(start_time, (void *)
                                              &SIMIX_process_create, arg);

  }
  if ((start_time < 0) || (start_time == SIMIX_get_clock())) {
    DEBUG2("Starting Process %s(%s) right now", parse_argv[0], parse_host);

    if (simix_global->create_process_function)
      process =
        (*simix_global->create_process_function) (parse_argv[0], parse_code,
                                                  NULL, parse_host,
                                                  parse_argc, parse_argv,
                                                  /*the props */
                                                  current_property_set);
    else
      process = SIMIX_process_create(parse_argv[0], parse_code, NULL, parse_host, parse_argc, parse_argv,       /*the props */
                                     current_property_set);

    if (process && kill_time > SIMIX_get_clock()) {
      if (simix_global->kill_process_function)
	surf_timer_model->extension_public->set(start_time, (void *) 
						simix_global->kill_process_function,
						process);
      else
        surf_timer_model->extension_public->set(kill_time, (void *)
                                                &SIMIX_process_kill,
                                                (void *) process);
    }
    xbt_free(parse_host);
  }
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
  xbt_assert0(simix_global,
              "SIMIX_global_init has to be called before SIMIX_launch_application.");
  surf_parse_reset_parser();
  surfxml_add_callback(STag_surfxml_process_cb_list, parse_process_init);
  surfxml_add_callback(ETag_surfxml_argument_cb_list, parse_argument);
  surfxml_add_callback(STag_surfxml_prop_cb_list, parse_properties);
  surfxml_add_callback(ETag_surfxml_process_cb_list, parse_process_finalize);

  surf_parse_open(file);
  xbt_assert1((!surf_parse()), "Parse error in %s", file);
  surf_parse_close();
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
  xbt_assert0(simix_global,
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
  xbt_assert0(simix_global,
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
  xbt_assert0(simix_global,
              "SIMIX_global_init has to be called before SIMIX_get_registered_function.");

  xbt_main_func_t res =
    xbt_dict_get_or_null(simix_global->registered_functions, name);
  return res ? res : default_function;
}
