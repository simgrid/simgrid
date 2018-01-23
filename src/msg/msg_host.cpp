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



}
