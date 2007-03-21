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
m_host_t __MSG_host_create(const char *name,
			 void *workstation,
			 void *data)
{
  m_host_t host = xbt_new0(s_m_host_t,1);

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

/*
 * Real function for destroy a host.
 * MSG_host_destroy is just  a front_end that also removes it from 
 * msg_global->host
 */
void __MSG_host_destroy(m_host_t host)
{
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

/** \ingroup m_host_management
 * \brief Return the speed of the processor (in flop/s), regardless of 
    the current load on the machine.
 */
double MSG_get_host_speed(m_host_t h)
{
	return 0.0;
}

/** \ingroup msg_gos_functions
 * \brief Determine if a host is available.
 *
 * \param h host to test
 */
int MSG_host_is_avail (m_host_t h)
{
	return 0;
}
