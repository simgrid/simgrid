/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix,
				"Logging specific to SIMIX (hosts)");

/********************************* Host **************************************/
smx_host_t __SIMIX_host_create(const char *name,
			       void *workstation, void *data)
{
  smx_simdata_host_t simdata = xbt_new0(s_smx_simdata_host_t, 1);
  smx_host_t host = xbt_new0(s_smx_host_t, 1);
  s_smx_process_t proc;

  /* Host structure */
  host->name = xbt_strdup(name);
  host->simdata = simdata;
  host->data = data;

  simdata->host = workstation;

  simdata->process_list =
      xbt_swag_new(xbt_swag_offset(proc, host_proc_hookup));
  /* Update global variables */

  xbt_fifo_unshift(simix_global->host, host);

  return host;
}

/** 
 * \brief Set the user data of a #smx_host_t.
 *
 * This functions checks whether some data has already been associated to \a host or not and attach \a data to \a host if it is possible.
 *	\param host SIMIX host
 *	\param data User data
 *
 */
void SIMIX_host_set_data(smx_host_t host, void *data)
{
  xbt_assert0((host != NULL), "Invalid parameters");
  xbt_assert0((host->data == NULL), "Data already set");

  /* Assign data */
  host->data = data;

  return;
}

/**
 * \brief Return the user data of a #smx_host_t.
 *
 * This functions checks whether \a host is a valid pointer or not and return the user data associated to \a host if it is possible.
 * \param host SIMIX host
 */
void *SIMIX_host_get_data(smx_host_t host)
{
  xbt_assert0((host != NULL), "Invalid parameters");

  /* Return data */
  return (host->data);
}

/** 
 * \brief Return the name of the #smx_host_t.
 *
 * This functions checks whether \a host is a valid pointer or not and return its name.
 * \param host SIMIX host
 */
const char *SIMIX_host_get_name(smx_host_t host)
{

  xbt_assert0((host != NULL)
	      && (host->simdata != NULL), "Invalid parameters");

  /* Return data */
  return (host->name);
}

/** 
 * \brief Return the location on which the current process is executed.
 *
 * Return the host,  more details in #SIMIX_process_get_host
 * \return SIMIX host
 */
smx_host_t SIMIX_host_self(void)
{
  return SIMIX_process_get_host(SIMIX_process_self());
}

/*
 * Real function for destroy a host.
 * MSG_host_destroy is just  a front_end that also removes it from 
 * msg_global->host
 */
void __SIMIX_host_destroy(smx_host_t host)
{
  smx_simdata_host_t simdata = NULL;

  xbt_assert0((host != NULL), "Invalid parameters");


  /* Clean Simulator data */
  simdata = host->simdata;

  if (xbt_swag_size(simdata->process_list) != 0) {
    char *msg =
	bprintf("Shutting down host %s, but it's not empty:", host->name);
    char *tmp;
    smx_process_t process = NULL;

    xbt_swag_foreach(process, simdata->process_list) {
      tmp = bprintf("%s\n\t%s", msg, process->name);
      free(msg);
      msg = tmp;
    }
    THROW1(arg_error, 0, "%s", msg);
  }

  xbt_swag_free(simdata->process_list);

  free(simdata);

  /* Clean host structure */
  free(host->name);
  free(host);

  return;
}

/**
 * \brief Return the current number of #smx_host_t.
 *
 * \return Number of hosts
 */
int SIMIX_host_get_number(void)
{
  return (xbt_fifo_size(simix_global->host));
}

/**
 * \brief Return a array of all the #smx_host_t.
 *
 * \return List of all hosts
 */
smx_host_t *SIMIX_host_get_table(void)
{
  return ((smx_host_t *) xbt_fifo_to_array(simix_global->host));
}


/**
 * \brief Return the speed of the processor.
 *
 * Return the speed (in Mflop/s), regardless of the current load on the machine.
 * \param host SIMIX host
 * \return Speed
 */
double SIMIX_host_get_speed(smx_host_t host)
{
  xbt_assert0((host != NULL), "Invalid parameters");

  return (surf_workstation_resource->
	  extension_public->get_speed(host->simdata->host, 1.0));
}

/**
 * \brief Return the available speed of the processor.
 *
 * Return the available speed (in Mflop/s).
 * \return Speed
 */
double SIMIX_host_get_available_speed(smx_host_t host)
{
  xbt_assert0((host != NULL), "Invalid parameters");

  return (surf_workstation_resource->
	  extension_public->get_available_speed(host->simdata->host));
}

/**
 * \brief Return the host by its name
 *
 * Finds a smx_host_t using its name.
 * \param name The name of an host.
 * \return The corresponding host
 */
smx_host_t SIMIX_host_get_by_name(const char *name)
{
  xbt_fifo_item_t i = NULL;
  smx_host_t host = NULL;

  xbt_assert0(((simix_global != NULL)
	       && (simix_global->host != NULL)),
	      "Environment not set yet");

  xbt_fifo_foreach(simix_global->host, i, host, smx_host_t) {
    if (strcmp(host->name, name) == 0)
      return host;
  }
  return NULL;
}

/**
 * \brief Return the state of a workstation
 *
 * Return the state of a workstation. Two states are possible, 1 if the host is active or 0 if it has crashed.
 * \param host The SIMIX host
 * \return 1 if host is available or 0 if not.
 */
int SIMIX_host_get_state(smx_host_t host)
{
  xbt_assert0((host != NULL), "Invalid parameters");

  return (surf_workstation_resource->
	  extension_public->get_state(host->simdata->host));

}
