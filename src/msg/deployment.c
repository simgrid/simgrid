/*     $Id$      */
  
/* Copyright (c) 2002-2007 Arnaud Legrand.                                  */
/* Copyright (c) 2007 Bruno Donassolo.                                      */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
  
#include "msg/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"


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
	SIMIX_function_register_process_create(_MSG_process_create_from_SIMIX);
	SIMIX_function_register_process_cleanup(__MSG_process_cleanup);
	SIMIX_function_register_process_kill(_MSG_process_kill_from_SIMIX);
   
	SIMIX_launch_application(file);

	return;
}

/** \ingroup msg_easier_life
 * \brief Registers the main function of an agent in a global table.
 *
 * Registers a code function in a global table. 
 * This table is then used by #MSG_launch_application. 
 * \param name the reference name of the function.
 * \param code the function (must have the same prototype than the main function of any C program: int ..(int argc, char *argv[]))
 */
void MSG_function_register(const char *name,xbt_main_func_t code)
{
	SIMIX_function_register(name, code);
	return;
}

/** \ingroup msg_easier_life
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

