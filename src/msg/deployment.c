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

extern char *yytext;


/** \ingroup msg_easier_life
 * \brief An application deployer.
 *
 * Creates the process described in \a file.
 * @param file a file containing a simple description of the application.
 * The format of each line is the following : host_name function_name arguments
 * You can use C-style comments and C-style strings.
 * Here is a simple exemple
 * 
 \verbatim
<DEPLOYMENT>
 // Here is the master
 the-doors.ens-lyon.fr  master "moby.cri2000.ens-lyon.fr" canaria.ens-lyon.fr popc.ens-lyon.fr
 // And here are the slaves
 moby.cri2000.ens-lyon.fr slave
 canaria.ens-lyon.fr slave
 popc.ens-lyon.fr slave 
</DEPLOYMENT>
\endverbatim
 */
void MSG_launch_application(const char *file) 
{
  char *host_name = NULL;
  int argc = -1 ;
  char **argv = NULL;
  m_process_t process = NULL ;
  m_host_t host = NULL;
  m_process_code_t code = NULL;
  e_surf_token_t token;

  MSG_global_init();
  
  find_section(file, "DEPLOYMENT");

  while(1) {
    token = surf_parse();

    if (token == TOKEN_END_SECTION)
      break;
    if (token == TOKEN_NEWLINE)
      continue;

    if (token == TOKEN_WORD) {
      surf_parse_deployment_line(&host_name,&argc,&argv);
      xbt_assert0(argc,"No function to execute");

      code = MSG_get_registered_function(argv[0]);
      xbt_assert1(code, "Unknown function %s",argv[0]);

      host = MSG_get_host_by_name(host_name);
      xbt_assert1(host, "Unknown host %s",host_name);

      process = MSG_process_create_with_arguments(argv[0], code, NULL, host,argc,argv);
      argc=-1;
      argv=NULL;
      xbt_free(host_name); 
    }
    else {
      CRITICAL1("Parse error line %d\n", surf_line_pos);
      xbt_abort();
    }
  }

  close_section("DEPLOYMENT");
}

/** \ingroup msg_easier_life
 * \brief Registers a ::m_process_code_t code in a global table.
 *
 * Registers a code function in a global table. 
 * This table is then used by ::MSG_launch_application. 
 * \param name the reference name of the function.
 * \param code the function
 */
void MSG_function_register(const char *name,m_process_code_t code)
{
  MSG_global_init();

  xbt_dict_set(msg_global->registered_functions,name,code,NULL);
}

/** \ingroup msg_easier_life
 * \brief Registers a ::m_process_t code in a global table.
 *
 * Registers a code function in a global table. 
 * This table is then used by ::MSG_launch_application. 
 * \param name the reference name of the function.
 */
m_process_code_t MSG_get_registered_function(const char *name)
{
  m_process_code_t code = NULL;

  MSG_global_init();
 
  xbt_dict_get(msg_global->registered_functions,name,(void **) &code);

  return code;
}

/** \ingroup msg_easier_life
 * \brief Get the arguments of the current process.
 * \deprecated{Not useful since m_process_code_t is int (*)(int argc, char *argv[])}
 *
 * This functions returns the values set for the current process 
 * using ::MSG_set_arguments or ::MSG_launch_application.
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

