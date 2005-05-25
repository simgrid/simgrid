/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"private.h"
#include"xbt/sysdep.h"
#include "xbt/error.h"
#include "surf/surf_parse.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(deployment, msg,
				"Logging specific to MSG (environment)");

static int parse_argc = -1 ;
static char **parse_argv = NULL;
static m_process_code_t parse_code = NULL;
static m_host_t parse_host = NULL;
static double start_time = 0.0;
static double kill_time = -1.0;
  
static void parse_process_init(void)
{
  parse_host = MSG_get_host_by_name(A_process_host);
  xbt_assert1(parse_host, "Unknown host %s",A_process_host);
  parse_code = MSG_get_registered_function(A_process_function);
  xbt_assert1(parse_code, "Unknown function %s",A_process_function);
  parse_argc = 0 ;
  parse_argv = NULL;
  parse_argc++;
  parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
  parse_argv[(parse_argc) - 1] = xbt_strdup(A_process_function);
  surf_parse_get_double(&start_time,A_process_start_time);
  surf_parse_get_double(&kill_time,A_process_kill_time);
}

static void parse_argument(void)
{
  parse_argc++;
  parse_argv = xbt_realloc(parse_argv, (parse_argc) * sizeof(char *));
  parse_argv[(parse_argc) - 1] = xbt_strdup(A_argument_value);
}

static void parse_process_finalize(void)
{
  MSG_process_create_with_arguments(parse_argv[0], parse_code, NULL, parse_host,
				    parse_argc,parse_argv,start_time,kill_time);
}

/** \ingroup msg_easier_life
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
 * Have a look in the directory examples/msg/ to have a bigger example.
 */
void MSG_launch_application(const char *file) 
{
  xbt_assert0(msg_global,"MSG_global_init_args has to be called before MSG_launch_application.");
  STag_process_fun = parse_process_init;
  ETag_argument_fun = parse_argument;
  ETag_process_fun = parse_process_finalize;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()),"Parse error in %s",file);
  surf_parse_close();
}

/** \ingroup msg_easier_life
 * \brief Registers a #m_process_code_t code in a global table.
 *
 * Registers a code function in a global table. 
 * This table is then used by #MSG_launch_application. 
 * \param name the reference name of the function.
 * \param code the function
 */
void MSG_function_register(const char *name,m_process_code_t code)
{
  xbt_assert0(msg_global,"MSG_global_init_args has to be called before MSG_function_register.");

  xbt_dict_set(msg_global->registered_functions,name,code,NULL);
}

/** \ingroup msg_easier_life
 * \brief Registers a #m_process_t code in a global table.
 *
 * Registers a code function in a global table. 
 * This table is then used by #MSG_launch_application. 
 * \param name the reference name of the function.
 */
m_process_code_t MSG_get_registered_function(const char *name)
{
  m_process_code_t code = NULL;

  xbt_assert0(msg_global,"MSG_global_init_args has to be called before MSG_get_registered_function.");
 
  xbt_dict_get(msg_global->registered_functions,name,(void **) &code);

  return code;
}

/** \ingroup msg_easier_life
 * \brief Get the arguments of the current process.
 * \deprecated{Not useful since #m_process_code_t is int (*)(int argc, char *argv[])}
 *
 * This functions returns the values set for the current process 
 * using #MSG_set_arguments or #MSG_launch_application.
 * \param argc the number of arguments
 * \param argv the arguments table
 */
MSG_error_t MSG_get_arguments(int *argc, char ***argv)
{
  m_process_t process = MSG_process_self();
  simdata_process_t simdata = NULL;

  xbt_assert0((argc) && (argv), "Invalid parameters");
  simdata = process->simdata;
  *argc = simdata->argc;
  *argv = simdata->argv;

  return MSG_OK;
}

/** \ingroup msg_easier_life
 * \brief Set the arguments of a process.
 *
 * This functions sets the argument number and the arguments table for a
 * proces.
 * \param process is the process you want to modify
 * \param argc the number of arguments
 * \param argv the arguments table
 */
MSG_error_t MSG_set_arguments(m_process_t process,int argc, char *argv[])
{
  simdata_process_t simdata = NULL;

  xbt_assert0(0,"Deprecated ! Do not use anymore. "
	      "Use MSG_process_create_with_arguments instead.\n");

  return MSG_OK;
}

