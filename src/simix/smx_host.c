/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** \defgroup m_host_management Management functions of Hosts
 *  \brief This section describes the host structure of MSG
 * 
 *     \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Hosts" --> \endhtmlonly
 * (#m_host_t) and the functions for managing it.
 *  
 *  A <em>location</em> (or <em>host</em>) is any possible place where
 *  a process may run. Thus it may be represented as a
 *  <em>physical resource with computing capabilities</em>, some
 *  <em>mailboxes</em> to enable running process to communicate with
 *  remote ones, and some <em>private data</em> that can be only
 *  accessed by local process.
 *  \see m_host_t
 */

/********************************* Host **************************************/
smx_host_t __SIMIX_host_create(const char *name,
			 void *workstation,
			 void *data)
{
  simdata_host_t simdata = xbt_new0(s_simdata_host_t,1);
  smx_host_t host = xbt_new0(s_smx_host_t,1);
  s_smx_process_t proc;

  /* Host structure */
  host->name = xbt_strdup(name);
  host->simdata = simdata;
  host->data = data;

  simdata->host = workstation;

  simdata->process_list = xbt_swag_new(xbt_swag_offset(proc, host_proc_hookup));
  /* Update global variables */

  xbt_fifo_unshift(simix_global->host, host);

  return host;
}

/** \ingroup m_host_management
 *
 * \brief Set the user data of a #m_host_t.
 *
 * This functions checks whether some data has already been associated to \a host 
   or not and attach \a data to \a host if it is possible.
 */
SIMIX_error_t SIMIX_host_set_data(smx_host_t host, void *data)
{
  xbt_assert0((host!=NULL), "Invalid parameters");
  xbt_assert0((host->data == NULL), "Data already set");

  /* Assign data */
  host->data = data;

  return SIMIX_OK;
}

/** \ingroup m_host_management
 *
 * \brief Return the user data of a #m_host_t.
 *
 * This functions checks whether \a host is a valid pointer or not and return
   the user data associated to \a host if it is possible.
 */
void *SIMIX_host_get_data(smx_host_t host)
{

  xbt_assert0((host != NULL), "Invalid parameters");

  /* Return data */
  return (host->data);
}

/** \ingroup m_host_management
 *
 * \brief Return the name of the #m_host_t.
 *
 * This functions checks whether \a host is a valid pointer or not and return
   its name.
 */
const char *SIMIX_host_get_name(smx_host_t host)
{

  xbt_assert0((host != NULL) && (host->simdata != NULL), "Invalid parameters");

  /* Return data */
  return (host->name);
}

/** \ingroup m_host_management
 *
 * \brief Return the location on which the current process is executed.
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
  simdata_host_t simdata = NULL;

  xbt_assert0((host != NULL), "Invalid parameters");

 
  /* Clean Simulator data */
  simdata = host->simdata;

  xbt_assert0((xbt_swag_size(simdata->process_list)==0),
	      "Some process are still running on this host");
  xbt_swag_free(simdata->process_list);

  free(simdata);

  /* Clean host structure */
  free(host->name);
  free(host);

  return;
}

/** \ingroup m_host_management
 * \brief Return the current number of #m_host_t.
 */
int SIMIX_get_host_number(void)
{
  return (xbt_fifo_size(simix_global->host));
}

/** \ingroup m_host_management
 * \brief Return a array of all the #m_host_t.
 */
smx_host_t *SIMIX_get_host_table(void)
{
  return ((smx_host_t *)xbt_fifo_to_array(simix_global->host));
}


/** \ingroup m_host_management
 * \brief Return the speed of the processor (in Mflop/s), regardless of 
    the current load on the machine.
 */
double SIMIX_get_host_speed(smx_host_t h)
{
  xbt_assert0((h!= NULL), "Invalid parameters");

  return(surf_workstation_resource->
	 extension_public->get_speed(h->simdata->host,1.0));
}

/** \ingroup msg_gos_functions
 * \brief Determine if a host is available.
 *
 * \param h host to test
 */
int SIMIX_host_is_avail (smx_host_t h)
{
  e_surf_cpu_state_t cpustate;
  xbt_assert0((h!= NULL), "Invalid parameters");

  cpustate =
    surf_workstation_resource->extension_public->get_state(h->simdata->host);

  xbt_assert0((cpustate == SURF_CPU_ON || cpustate == SURF_CPU_OFF),
	      "Invalid cpu state");

  return (cpustate==SURF_CPU_ON);
}
