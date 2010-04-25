/* Copyright (c) 2002-2007 Arnaud Legrand.                                  */
/* Copyright (c) 2007 Bruno Donassolo.                                      */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "mailbox.h"

/** \defgroup m_host_management Management functions of Hosts
 *  \brief This section describes the host structure of MSG
 */
/** @addtogroup m_host_management
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
m_host_t __MSG_host_create(smx_host_t workstation, void *data)
{
  const char *name;
  simdata_host_t simdata = xbt_new0(s_simdata_host_t, 1);
  m_host_t host = xbt_new0(s_m_host_t, 1);
  int i;

  char alias[MAX_ALIAS_NAME + 1] = { 0 };       /* buffer used to build the key of the mailbox */

  name = SIMIX_host_get_name(workstation);
  /* Host structure */
  host->name = xbt_strdup(name);
  host->simdata = simdata;
  host->data = data;

  simdata->smx_host = workstation;

  if (msg_global->max_channel > 0)
    simdata->mailboxes = xbt_new0(msg_mailbox_t, msg_global->max_channel);

  for (i = 0; i < msg_global->max_channel; i++) {
    sprintf(alias, "%s:%d", name, i);

    /* the key of the mailbox (in this case) is build from the name of the host and the channel number */
    simdata->mailboxes[i] = MSG_mailbox_create(alias);
    memset(alias, 0, MAX_ALIAS_NAME + 1);
  }

  simdata->mutex = SIMIX_mutex_init();
  SIMIX_host_set_data(workstation, host);

  /* Update global variables */
  xbt_fifo_unshift(msg_global->host, host);

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
  xbt_assert0((host != NULL), "Invalid parameters");
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

  xbt_assert0((host != NULL)
              && (host->simdata != NULL), "Invalid parameters");

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

/*
 * Real function to destroy a host.
 * MSG_host_destroy is just  a front_end that also removes it from 
 * msg_global->host
 */
void __MSG_host_destroy(m_host_t host)
{
  simdata_host_t simdata = NULL;
  int i = 0;
  char alias[MAX_ALIAS_NAME + 1] = { 0 };       /* buffer used to build the key of the mailbox */

  xbt_assert0((host != NULL), "Invalid parameters");

  /* Clean Simulator data */
  /* SIMIX host will be cleaned when MSG_clean calls SIMIX_clean */
  simdata = (host)->simdata;

  for (i = 0; i < msg_global->max_channel; i++) {
    sprintf(alias, "%s:%d", host->name, i);
    MSG_mailbox_free((void *) (simdata->mailboxes[i]));
    memset(alias, 0, MAX_ALIAS_NAME + 1);
  }

  if (msg_global->max_channel > 0)
    free(simdata->mailboxes);
  SIMIX_mutex_destroy(simdata->mutex);
  free(simdata);

  /* Clean host structure */
  free(host->name);
  free(host);


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
  return ((m_host_t *) xbt_fifo_to_array(msg_global->host));
}

/** \ingroup m_host_management
 * \brief Return the number of MSG tasks currently running on a
 * #m_host_t. The external load is not taken in account.
 */
int MSG_get_host_msgload(m_host_t h)
{
  xbt_assert0((h != NULL), "Invalid parameters");
  xbt_assert0(0, "Not implemented yet");

  return (0);
}

/** \ingroup m_host_management
 * \brief Return the speed of the processor (in flop/s), regardless of 
    the current load on the machine.
 */
double MSG_get_host_speed(m_host_t h)
{
  xbt_assert0((h != NULL), "Invalid parameters");

  return (SIMIX_host_get_speed(h->simdata->smx_host));
}

/** \ingroup m_host_management
 * \brief Returns the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \return value of a property (or NULL if property not set)
 */
const char *MSG_host_get_property_value(m_host_t host, const char *name)
{
  return xbt_dict_get_or_null(MSG_host_get_properties(host), name);
}

/** \ingroup m_host_management
 * \brief Returns a xbt_dynar_t consisting of the list of properties assigned to this host
 *
 * \param host a host
 * \return a dict containing the properties
 */
xbt_dict_t MSG_host_get_properties(m_host_t host)
{
  xbt_assert0((host != NULL), "Invalid parameters (host is NULL)");

  return (SIMIX_host_get_properties(host->simdata->smx_host));
}


/** \ingroup msg_gos_functions
 * \brief Determine if a host is available.
 *
 * \param h host to test
 */
int MSG_host_is_avail(m_host_t h)
{
  xbt_assert0((h != NULL), "Invalid parameters (host is NULL)");
  return (SIMIX_host_get_state(h->simdata->smx_host));
}
