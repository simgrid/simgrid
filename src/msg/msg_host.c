/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.h"
#include "src/msg/msg_mailbox.h"
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
msg_host_t __MSG_host_create(sg_host_t host) // FIXME: don't return our parameter
{
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
  priv->is_migrating = 0;

  priv->affinity_mask_db = xbt_dict_new_homogeneous(NULL);

  priv->file_descriptor_table = xbt_dynar_new(sizeof(int), NULL);
  for (int i=1023; i>=0;i--)
    xbt_dynar_push_as(priv->file_descriptor_table, int, i);

  sg_host_msg_set(host,priv);
  
  return host;
}

/** \ingroup m_host_management
 * \brief Finds a msg_host_t using its name.
 *
 * This is a name directory service
 * \param name the name of an host.
 * \return the corresponding host
 */
msg_host_t MSG_host_by_name(const char *name)
{
  return (msg_host_t) xbt_lib_get_elm_or_null(host_lib,name);
}

/** \ingroup m_host_management
 *
 * \brief Set the user data of a #msg_host_t.
 *
 * This functions checks whether some data has already been associated to \a host
   or not and attach \a data to \a host if it is possible.
 */
msg_error_t MSG_host_set_data(msg_host_t host, void *data) {
  sg_host_user_set(host, data);
  return MSG_OK;
}

/** \ingroup m_host_management
 *
 * \brief Return the user data of a #msg_host_t.
 *
 * This functions checks whether \a host is a valid pointer or not and return
   the user data associated to \a host if it is possible.
 */
void *MSG_host_get_data(msg_host_t host) {
  return sg_host_user(host);
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


/** \ingroup m_host_management
 *
 * \brief Start the host if it is off
 *
 * See also #MSG_host_is_on() and #MSG_host_is_off() to test the current state of the host and @ref SURF_plugin_energy for more info on DVFS.
 */
void MSG_host_on(msg_host_t host)
{
  simcall_host_on(host);
}

/** \ingroup m_host_management
 *
 * \brief Stop the host if it is on
 *
 * See also #MSG_host_is_on() and #MSG_host_is_off() to test the current state of the host and @ref SURF_plugin_energy for more info on DVFS.
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

  if (priv == NULL)
	  return;
  unsigned int size = xbt_dict_size(priv->dp_objs);
  if (size > 0)
    XBT_WARN("dp_objs: %u pending task?", size);
  xbt_dict_free(&priv->dp_objs);
  xbt_dict_free(&priv->affinity_mask_db);
  xbt_dynar_free(&priv->file_descriptor_table);
#ifdef MSG_USE_DEPRECATED
  free(priv->mailboxes);
#endif

  free(priv);
}

/*
 * \brief Destroys a host (internal call only)
 */
void __MSG_host_destroy(msg_host_t host) //FIXME: killme?
{
  /* TODO:
   * What happens if VMs still remain on this host?
   * Revisit here after the surf layer gets stable.
   **/
  sg_host_msg_destroy(host);
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
 * \remark The host order in the returned array is generally different from the host creation/declaration order in the XML platform (we use a hash table internally)
 */
xbt_dynar_t MSG_hosts_as_dynar(void) {
  return sg_hosts_as_dynar();
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

  return sg_host_get_speed(h);
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

  return sg_host_get_core(host);
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


/** @ingroup m_host_management
 *
 * @brief Determine if a host is up and running.
 *
 * See also #MSG_host_on() and #MSG_host_off() to switch the host ON and OFF and @ref SURF_plugin_energy for more info on DVFS.
 *
 * @param host host to test
 * @return Returns true if the host is up and running, and false if it's currently down
 */
int MSG_host_is_on(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
  return sg_host_get_state(host);
}
/** @ingroup m_host_management
 *
 * @brief Determine if a host is currently off.
 *
 * See also #MSG_host_on() and #MSG_host_off() to switch the host ON and OFF and @ref SURF_plugin_energy for more info on DVFS.
 */
int MSG_host_is_off(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
  return !(sg_host_get_state(host));
}

/** \ingroup m_host_management
 * \brief Set the parameters of a given host
 *
 * \param host a host
 * \param params a prameter object
 */
void MSG_host_set_params(msg_host_t host, vm_params_t params)
{
  simcall_host_set_params(host, params);
}

/** \ingroup m_host_management
 * \brief Get the parameters of a given host
 *
 * \param host a host
 * \param params a prameter object
 */
void MSG_host_get_params(msg_host_t host, vm_params_t params)
{
  simcall_host_get_params(host, params);
}

/** \ingroup m_host_management
 * \brief Return the speed of the processor (in flop/s) at a given pstate. See also @ref SURF_plugin_energy.
 *
 * \param  host host to test
 * \param pstate_index pstate to test
 * \return Returns the processor speed associated with pstate_index
 */
double MSG_host_get_power_peak_at(msg_host_t host, int pstate_index) {
	  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
	  return (simcall_host_get_power_peak_at(host, pstate_index));
}

/** \ingroup m_host_management
 * \brief Return the current speed of the processor (in flop/s)
 *
 * \param  host host to test
 * \return Returns the current processor speed
 */
double MSG_host_get_current_power_peak(msg_host_t host) {
	  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
	  return simcall_host_get_current_power_peak(host);
}

/** \ingroup m_host_management
 * \brief Return the total count of pstates defined for a host. See also @ref SURF_plugin_energy.
 *
 * \param  host host to test
 */
int MSG_host_get_nb_pstates(msg_host_t host) {
	  return sg_host_get_nb_pstates(host);
}

/** \ingroup m_host_management
 * \brief Sets the speed of the processor (in flop/s) at a given pstate. See also @ref SURF_plugin_energy.
 *
 * \param  host host to test
 * \param pstate_index pstate to switch to
 */
void MSG_host_set_pstate(msg_host_t host, int pstate_index) {
	  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");

	  simcall_host_set_pstate(host, pstate_index);
}
/** \ingroup m_host_management
 * \brief Gets the pstate at which the given host is currently running. See also @ref SURF_plugin_energy.
 *
 * \param  host host to test
 */
int MSG_host_get_pstate(msg_host_t host) {
	  return sg_host_get_pstate(host);
}

/** \ingroup m_host_management
 * \brief Return the total energy consumed by a host (in Joules). See also @ref SURF_plugin_energy.
 *
 * \param  host host to test
 * \return Returns the consumed energy
 */
double MSG_host_get_consumed_energy(msg_host_t host) {
	  xbt_assert((host != NULL), "Invalid parameters (host is NULL)");
	  return sg_host_get_consumed_energy(host);
}
/** \ingroup m_host_management
 * \brief Returns the amount of watt dissipated at the given pstate when the host is idling
 *
 */
double MSG_host_get_wattmin_at(msg_host_t host, int pstate){
	return simcall_host_get_wattmin_at(host, pstate);
}
/** \ingroup m_host_management
 * \brief Returns the amount of watt dissipated at the given pstate when the host burns CPU at 100%
 *
 */
double MSG_host_get_wattmax_at(msg_host_t host, int pstate){
	return simcall_host_get_wattmax_at(host, pstate);
}

/** \ingroup m_host_management
 * \brief Return the list of mount point names on an host.
 * \param host a host
 * \return a dict containing all mount point on the host (mount_name => msg_storage_t)
 */
xbt_dict_t MSG_host_get_mounted_storage_list(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters");
  return (simcall_host_get_mounted_storage_list(host));
}

/** \ingroup m_host_management
 * \brief Return the list of storages attached to an host.
 * \param host a host
 * \return a dynar containing all storages (name) attached to the host
 */
xbt_dynar_t MSG_host_get_attached_storage_list(msg_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters");
  return (simcall_host_get_attached_storage_list(host));
}

/** \ingroup m_host_management
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

  xbt_dict_t storage_list = simcall_host_get_mounted_storage_list(host);

  xbt_dict_foreach(storage_list,cursor,mount_name,storage_name){
    storage = (msg_storage_t)xbt_lib_get_elm_or_null(storage_lib,storage_name);
    xbt_dict_t content = simcall_storage_get_content(storage);
    xbt_dict_set(contents,mount_name, content,NULL);
  }
  xbt_dict_free(&storage_list);
  return contents;
}

int __MSG_host_get_file_descriptor_id(msg_host_t host){
  msg_host_priv_t priv = sg_host_msg(host);
  xbt_assert(!xbt_dynar_is_empty(priv->file_descriptor_table),
    "Too much files are opened! Some have to be closed.");
  return xbt_dynar_pop_as(priv->file_descriptor_table, int);
}

void __MSG_host_release_file_descriptor_id(msg_host_t host, int id){
  msg_host_priv_t priv = sg_host_msg(host);
  xbt_dynar_push_as(priv->file_descriptor_table, int, id);
}
