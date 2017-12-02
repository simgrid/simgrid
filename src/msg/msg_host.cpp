/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_host_private.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg);

extern "C" {

/** @addtogroup m_host_management
 * (#msg_host_t) and the functions for managing it.
 *
 *  A <em>location</em> (or <em>host</em>) is any possible place where  a process may run. Thus it may be represented
 *  as a <em>physical resource with computing capabilities</em>, some <em>mailboxes</em> to enable running process to
 *  communicate with remote ones, and some <em>private data</em> that can be only accessed by local process.
 *  \see msg_host_t
 */

/********************************* Host **************************************/
/** \ingroup m_host_management
 * \brief Finds a msg_host_t using its name.
 *
 * This is a name directory service
 * \param name the name of an host.
 * \return the corresponding host
 */
msg_host_t MSG_host_by_name(const char *name)
{
  return simgrid::s4u::Host::by_name_or_null(name);
}

/** \ingroup m_host_management
 *
 * \brief Set the user data of a #msg_host_t.
 *
 * This functions attach \a data to \a host if it is possible.
 */
msg_error_t MSG_host_set_data(msg_host_t host, void *data) {
  sg_host_user_set(host, data);
  return MSG_OK;
}

/** \ingroup m_host_management
 *
 * \brief Return the user data of a #msg_host_t.
 *
 * This functions returns the user data associated to \a host if it is possible.
 */
void *MSG_host_get_data(msg_host_t host) {
  return sg_host_user(host);
}

/** \ingroup m_host_management
 *
 * \brief Return the location on which the current process is executed.
 */
msg_host_t MSG_host_self()
{
  return MSG_process_get_host(nullptr);
}

/** \ingroup m_host_management
 *
 * \brief Start the host if it is off
 *
 * See also #MSG_host_is_on() and #MSG_host_is_off() to test the current state of the host and @ref plugin_energy
 * for more info on DVFS.
 */
void MSG_host_on(msg_host_t host)
{
  host->turnOn();
}

/** \ingroup m_host_management
 *
 * \brief Stop the host if it is on
 *
 * See also #MSG_host_is_on() and #MSG_host_is_off() to test the current state of the host and @ref plugin_energy
 * for more info on DVFS.
 */
void MSG_host_off(msg_host_t host)
{
  host->turnOff();
}

/** \ingroup m_host_management
 * \brief Return the current number MSG hosts.
 */
int MSG_get_host_number()
{
  return sg_host_count();
}

/** \ingroup m_host_management
 * \brief Return a dynar containing all the hosts declared at a given point of time (including VMs)
 * \remark The host order in the returned array is generally different from the host creation/declaration order in the
 *         XML platform (we use a hash table internally)
 */
xbt_dynar_t MSG_hosts_as_dynar() {
  return sg_hosts_as_dynar();
}

/** \ingroup m_host_management
 * \brief Return the speed of the processor (in flop/s), regardless of the current load on the machine.
 */
double MSG_host_get_speed(msg_host_t host) {
  return host->getSpeed();
}

/** \ingroup m_host_management
 * \brief Return the number of cores.
 *
 * \param host a host
 * \return the number of cores
 */
int MSG_host_get_core_number(msg_host_t host) {
  return host->getCoreCount();
}

/** \ingroup m_host_management
 * \brief Return the list of processes attached to an host.
 *
 * \param host a host
 * \param whereto a dynar in which we should push processes living on that host
 */
void MSG_host_get_process_list(msg_host_t host, xbt_dynar_t whereto)
{
  xbt_assert((host != nullptr), "Invalid parameters");
  for (auto& actor : host->extension<simgrid::simix::Host>()->process_list) {
    msg_process_t p = actor.ciface();
    xbt_dynar_push(whereto, &p);
  }
}

/** \ingroup m_host_management
 * \brief Returns the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \return value of a property (or nullptr if property not set)
 */
const char *MSG_host_get_property_value(msg_host_t host, const char *name)
{
  return host->getProperty(name);
}

/** \ingroup m_host_management
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to this host
 *
 * \param host a host
 * \return a dict containing the properties
 */
xbt_dict_t MSG_host_get_properties(msg_host_t host)
{
  xbt_assert((host != nullptr), "Invalid parameters (host is nullptr)");
  xbt_dict_t as_dict = xbt_dict_new_homogeneous(xbt_free_f);
  std::map<std::string, std::string>* props = host->getProperties();
  if (props == nullptr)
    return nullptr;
  for (auto const& elm : *props) {
    xbt_dict_set(as_dict, elm.first.c_str(), xbt_strdup(elm.second.c_str()), nullptr);
  }
  return as_dict;
}

/** \ingroup m_host_management
 * \brief Change the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \param value what to change the property to
 */
void MSG_host_set_property_value(msg_host_t host, const char* name, char* value)
{
  host->setProperty(name, value);
}

/** @ingroup m_host_management
 * @brief Determine if a host is up and running.
 *
 * See also #MSG_host_on() and #MSG_host_off() to switch the host ON and OFF and @ref plugin_energy for more info on
 * DVFS.
 *
 * @param host host to test
 * @return Returns true if the host is up and running, and false if it's currently down
 */
int MSG_host_is_on(msg_host_t host)
{
  return host->isOn();
}

/** @ingroup m_host_management
 * @brief Determine if a host is currently off.
 *
 * See also #MSG_host_on() and #MSG_host_off() to switch the host ON and OFF and @ref plugin_energy for more info on
 * DVFS.
 */
int MSG_host_is_off(msg_host_t host)
{
  return host->isOff();
}

/** \ingroup m_host_management
 * \brief Return the speed of the processor (in flop/s) at a given pstate. See also @ref plugin_energy.
 *
 * \param  host host to test
 * \param pstate_index pstate to test
 * \return Returns the processor speed associated with pstate_index
 */
double MSG_host_get_power_peak_at(msg_host_t host, int pstate_index) {
  xbt_assert((host != nullptr), "Invalid parameters (host is nullptr)");
  return host->getPstateSpeed(pstate_index);
}

/** \ingroup m_host_management
 * \brief Return the total count of pstates defined for a host. See also @ref plugin_energy.
 *
 * \param  host host to test
 */
int MSG_host_get_nb_pstates(msg_host_t host) {
  return sg_host_get_nb_pstates(host);
}

/** \ingroup m_host_management
 * \brief Return the list of mount point names on an host.
 * \param host a host
 * \return a dict containing all mount point on the host (mount_name => msg_storage_t)
 */
xbt_dict_t MSG_host_get_mounted_storage_list(msg_host_t host)
{
  return sg_host_get_mounted_storage_list(host);
}

/** \ingroup m_host_management
 * \brief Return the list of storages attached to an host.
 * \param host a host
 * \return a dynar containing all storages (name) attached to the host
 */
xbt_dynar_t MSG_host_get_attached_storage_list(msg_host_t host)
{
  return sg_host_get_attached_storage_list(host);
}

}
