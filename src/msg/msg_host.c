/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg_private.h"
#include "msg/msg_mailbox.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "simgrid/simix.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg);

/** @addtogroup m_host_management
 * \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Hosts" --> \endhtmlonly
 * (#msg_host_t) and the functions for managing it.
 *  
 *  A <em>location</em> (or <em>host</em>) is any possible place where
 *  a process may run. Thus it may be represented as a
 *  <em>physical resource with computing capabilities</em>, some
 *  <em>mailboxes</em> to enable running process to communicate with
 *  remote ones, and some <em>private data</em> that can be only
 *  accessed by local process.
 *  \see msg_host_t
 */

/********************************* Host **************************************/
msg_host_t __MSG_host_create(smx_host_t workstation)
{
  const char *name = SIMIX_host_get_name(workstation);
  msg_host_priv_t priv = xbt_new0(s_msg_host_priv_t, 1);

#ifdef MSG_USE_DEPRECATED
  int i;
  char alias[MAX_ALIAS_NAME + 1] = { 0 };       /* buffer used to build the key of the mailbox */

  priv->mailboxes = (msg_global->max_channel > 0) ?
    xbt_new0(msg_mailbox_t, msg_global->max_channel) : NULL;

  for (i = 0; i < msg_global->max_channel; i++) {
    sprintf(alias, "%s:%d", name, i);

    /* the key of the mailbox (in this case) is build from the name of the host and the channel number */
    priv->mailboxes[i] = MSG_mailbox_new(alias);
    memset(alias, 0, MAX_ALIAS_NAME + 1);
  }
#endif


  priv->dp_objs = xbt_dict_new();
  priv->dp_enabled = 0;
  priv->dp_updated_by_deleted_tasks = 0;

  priv->affinity_mask_db = xbt_dict_new_homogeneous(NULL);

  xbt_lib_set(host_lib, name, MSG_HOST_LEVEL, priv);
  
  return xbt_lib_get_elm_or_null(host_lib, name);
}


/** \ingroup msg_host_management
 * \brief Finds a msg_host_t using its name.
 *
 * This is a name directory service
 * \param name the name of an host.
 * \return the corresponding host
 */
msg_host_t MSG_get_host_by_name(const char *name)
{
  return (msg_host_t) xbt_lib_get_elm_or_null(host_lib,name);
}

/** \ingroup m_host_management
 *
 * \brief Return the name of the #msg_host_t.
 *
 * This functions checks whether \a host is a valid pointer or not and return
   its name.
 */
const char *MSG_host_get_name(msg_host_t host) {
  return SIMIX_host_get_name(host);
}

/** \ingroup m_host_management
 *
 * \brief Return the location on which the current process is executed.
 */
msg_host_t MSG_host_self(void)
{
  return MSG_process_get_host(NULL);
}


/*
 * \brief Start the host if it is off
 */
void MSG_host_on(msg_host_t host)
{
  simcall_host_on(host);
}

/*
 * \brief Stop the host if it is on
 */
void MSG_host_off(msg_host_t host)
{
  simcall_host_off(host);
}

/*
 * \brief Frees private data of a host (internal call only)
 */
void __MSG_host_priv_free(msg_host_priv_t priv)
{
  unsigned int size = xbt_dict_size(priv->dp_objs);
  if (size > 0)
    XBT_WARN("dp_objs: %u pending task?", size);
  xbt_dict_free(&priv->dp_objs);
  xbt_dict_free(&priv->affinity_mask_db);

#ifdef MSG_USE_DEPRECATED
  free(priv->mailboxes);
#endif

  free(priv);
}

/*
 * \brief Destroys a host (internal call only)
 */
void __MSG_host_destroy(msg_host_t host)
{
  const char *name = MSG_host_get_name(host);
  /* TODO:
   * What happens if VMs still remain on this host?
   * Revisit here after the surf layer gets stable.
   **/

  xbt_lib_unset(host_lib, name, MSG_HOST_LEVEL, 1);
}

/** \ingroup m_host_management
 * \brief Return the current number MSG hosts.
 */
int MSG_get_host_number(void)
{
  return xbt_lib_length(host_lib);
}

#ifdef MSG_USE_DEPRECATED
msg_host_t *MSG_get_host_table(void)
{
      void **array;
    int i = 0;
    xbt_lib_cursor_t cursor;
    char *key;
    void **data;

    if (xbt_lib_length(host_lib) == 0)
    return NULL;
    else
    array = xbt_new0(void *, xbt_lib_length(host_lib));

    xbt_lib_foreach(host_lib, cursor, key, data) {
      if(routing_get_network_element_type(key) == SURF_NETWORK_ELEMENT_HOST)
        array[i++] = data[MSG_HOST_LEVEL];
    }

    return (msg_host_t *)array;
}
#endif

/** \ingroup m_host_management
 * \brief Return a dynar containing all the hosts declared at a given point of time
 */
xbt_dynar_t MSG_hosts_as_dynar(void) {
  xbt_lib_cursor_t cursor;
  char *key;
  void **data;
  xbt_dynar_t res = xbt_dynar_new(sizeof(msg_host_t),NULL);

  xbt_lib_foreach(host_lib, cursor, key, data) {
    if(routing_get_network_element_type(key) == SURF_NETWORK_ELEMENT_HOST) {
      xbt_dictelm_t elm = xbt_dict_cursor_get_elm(cursor);
      xbt_dynar_push(res, &elm);
    }
  }
  return res;
}

/** \ingroup m_host_management
 * \brief Return the number of MSG tasks currently running on a
 * #msg_host_t. The external load is not taken in account.
 */
int MSG_get_host_msgload(msg_host_t h)
{
  xbt_assert((h != NULL), "Invalid parameters");
  xbt_die( "Not implemented yet");

  return (0);
}

/** \ingroup m_host_management
 * \brief Return the speed of the processor (in flop/s), regardless of 
    the current load on the machine.
 */
double MSG_get_host_speed(msg_host_t h)
{
  xbt_assert((h != NULL), "Invalid parameters");

  return (simcall_host_get_speed(h));
}


/** \ingroup m_host_management
 * \brief Return the number of cores.
 *
 * \param host a host
 * \return the number of cores
 */
int MSG_host_get_core_number(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters");

  return (simcall_host_get_core(host));
}

/** \ingroup m_host_management
 * \brief Return the list of processes attached to an host.
 *
 * \param host a host
 * \return a swag with the attached processes
 */
xbt_swag_t MSG_host_get_process_list(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters");

  return (simcall_host_get_process_list(host));
}


/** \ingroup m_host_management
 * \brief Returns the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \return value of a property (or NULL if property not set)
 */
const char *MSG_host_get_property_value(msg_host_t host, const char *name)
{
  return xbt_dict_get_or_null(MSG_host_get_properties(host), name);
}

/** \ingroup m_host_management
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to this host
 *
 * \param host a host
 * \return a dict containing the properties
 */
xbt_dict_t MSG_host_get_properties(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");

  return (simcall_host_get_properties(host));
}

/** \ingroup m_host_management
 * \brief Change the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \param value what to change the property to
 * \param free_ctn the freeing function to use to kill the value on need
 */
void MSG_host_set_property_value(msg_host_t host, const char *name, char *value,void_f_pvoid_t free_ctn) {

  xbt_dict_set(MSG_host_get_properties(host), name, value,free_ctn);
}


/** \ingroup msg_gos_functions
 * \brief Determine if a host is available.
 *
 * \param host host to test
 * \return Returns 1 if host is available, 0 otherwise
 */
int MSG_host_is_avail(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
  return (simcall_host_get_state(host));
}

/** \ingroup m_host_management
 * \brief Set the parameters of a given host
 *
 * \param host a host
 * \param params a prameter object
 */
void MSG_host_set_params(msg_host_t host, ws_params_t params)
{
  simcall_host_set_params(host, params);
}

/** \ingroup m_host_management
 * \brief Get the parameters of a given host
 *
 * \param host a host
 * \param params a prameter object
 */
void MSG_host_get_params(msg_host_t host, ws_params_t params)
{
  simcall_host_get_params(host, params);
}

/** \ingroup m_host_management
 * \brief Return the speed of the processor (in flop/s) at a given pstate
 *
 * \param  host host to test
 * \param pstate_index pstate to test
 * \return Returns the processor speed associated with pstate_index
 */
double MSG_get_host_power_peak_at(msg_host_t host, int pstate_index) {
	  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
	  return (simcall_host_get_power_peak_at(host, pstate_index));
}

/** \ingroup m_host_management
 * \brief Return the current speed of the processor (in flop/s)
 *
 * \param  host host to test
 * \return Returns the current processor speed
 */
double MSG_get_host_current_power_peak(msg_host_t host) {
	  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
	  return simcall_host_get_current_power_peak(host);
}

/** \ingroup m_host_management
 * \brief Return the number of pstates defined for a host
 *
 * \param  host host to test
 */
int MSG_get_host_nb_pstates(msg_host_t host) {

	  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
	  return (simcall_host_get_nb_pstates(host));
}

/** \ingroup m_host_management
 * \brief Sets the speed of the processor (in flop/s) at a given pstate
 *
 * \param  host host to test
 * \param pstate_index pstate to switch to
 */
void MSG_set_host_power_peak_at(msg_host_t host, int pstate_index) {
	  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");

	  simcall_host_set_power_peak_at(host, pstate_index);
}

/** \ingroup m_host_management
 * \brief Return the total energy consumed by a host (in Joules)
 *
 * \param  host host to test
 * \return Returns the consumed energy
 */
double MSG_get_host_consumed_energy(msg_host_t host) {
	  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
	  return simcall_host_get_consumed_energy(host);
}

/** \ingroup m_host_management
 * \brief Return the list of mount point names on an host.
 * \param host a host
 * \return a dict containing all mount point on the host (mount_name => msg_storage_t)
 */
xbt_dict_t MSG_host_get_storage_list(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters");
  return (simcall_host_get_storage_list(host));
}

/** \ingroup msg_host_management
 * \brief Return the content of mounted storages on an host.
 * \param host a host
 * \return a dict containing content (as a dict) of all storages mounted on the host
 */
xbt_dict_t MSG_host_get_storage_content(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters");
  xbt_dict_t contents = xbt_dict_new_homogeneous(NULL);
  msg_storage_t storage;
  char* storage_name;
  char* mount_name;
  xbt_dict_cursor_t cursor = NULL;

  xbt_dict_t storage_list = simcall_host_get_storage_list(host);

  xbt_dict_foreach(storage_list,cursor,mount_name,storage_name){
    storage = (msg_storage_t)xbt_lib_get_elm_or_null(storage_lib,storage_name);
    xbt_dict_t content = simcall_storage_get_content(storage);
    xbt_dict_set(contents,mount_name, content,NULL);
  }
  xbt_dict_free(&storage_list);
  return contents;
}
