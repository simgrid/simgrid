/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"private.h"
#include"xbt/sysdep.h"
#include "xbt/error.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(host, msg,
				"Logging specific to MSG (host)");

/********************************* Host **************************************/
m_host_t __MSG_host_create(const char *name,
			 void *workstation,
			 void *data)
{
  sim_data_host_t sim_data = xbt_new0(s_sim_data_host_t,1);
  m_host_t host = xbt_new0(s_m_host_t,1);
  int i;

  /* Host structure */
  host->name = xbt_strdup(name);
  host->simdata = sim_data;
  host->data = data;

  sim_data->host = workstation;

  sim_data->mbox = xbt_new0(xbt_fifo_t, msg_global->max_channel);
  for (i = 0; i < msg_global->max_channel; i++)
    sim_data->mbox[i] = xbt_fifo_new();
  sim_data->sleeping = xbt_new0(m_process_t, msg_global->max_channel);
  /* Update global variables */

  xbt_fifo_push(msg_global->host, host);

  return host;
}

/** \ingroup m_host_management
 *
 * \brief Set the user data of a #m_host_t.
 *
 * This functions checks whether some data has already been associated to \a host 
   or not and attach \a data to \a host if it is possible.
 */
MSG_error_t MSG_host_set_data(m_host_t host, void *data)
{
  xbt_assert0((host!=NULL), "Invalid parameters");
  xbt_assert0((host->data == NULL), "Data already set");

  /* Assign data */
  host->data = data;

  return MSG_OK;
}

/** \ingroup m_host_management
 *
 * \brief Return the user data of a #m_host_t.
 *
 * This functions checks whether \a host is a valid pointer or not and return
   the user data associated to \a host if it is possible.
 */
void *MSG_host_get_data(m_host_t host)
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
const char *MSG_host_get_name(m_host_t host)
{

  xbt_assert0((host != NULL) && (host->simdata != NULL), "Invalid parameters");

  /* Return data */
  return (host->name);
}

/** \ingroup m_host_management
 *
 * \brief Return the location on which the current process is executed.
 */
m_host_t MSG_host_self(void)
{
  return MSG_process_get_host(MSG_process_self());
}

/**
 * Real function for destroy a host.
 * MSG_host_destroy is just  a front_end that also removes it from 
 * msg_global->host
 */
void __MSG_host_destroy(m_host_t host)
{
  sim_data_host_t sim_data = NULL;
  int i = 0;

  xbt_assert0((host != NULL), "Invalid parameters");

  /* Clean Simulator data */
  sim_data = (host)->simdata;

  for (i = 0; i < msg_global->max_channel; i++)
    xbt_fifo_free(sim_data->mbox[i]);
  xbt_free(sim_data->mbox);
  xbt_free(sim_data->sleeping);
  xbt_assert0((xbt_swag_size(&(sim_data->process_list))==0),
	      "Some process are still running on this host");

  xbt_free(sim_data);

  /* Clean host structure */
  xbt_free(host->name);
  xbt_free(host);

  return;
}

/** \ingroup m_host_management
 * \brief Return the current number of #m_host_t.
 */
int MSG_get_host_number(void)
{
  return (xbt_fifo_size(msg_global->host));
}

/** \ingroup m_host_management
 * \brief Return a array of all the #m_host_t.
 */
m_host_t *MSG_get_host_table(void)
{
  return ((m_host_t *)xbt_fifo_to_array(msg_global->host));
}

/** \ingroup m_host_management
 * \brief Return the number of MSG tasks currently running on a
 * #m_host_t. The external load is not taken in account.
 */
int MSG_get_host_msgload(m_host_t h)
{
  xbt_assert0((h!= NULL), "Invalid parameters");
  xbt_assert0(0, "Not implemented yet");

  return(0);
/*   return(surf_workstation_resource->extension_public->get_load(h->simdata->host)); */
}
