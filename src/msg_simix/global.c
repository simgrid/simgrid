#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h" /* ex_backtrace_display */

MSG_Global_t msg_global = NULL;

/* static void MarkAsFailed(m_task_t t, TBX_HashTable_t failedProcessList); */
/* static xbt_fifo_t MSG_buildFailedHostList(double a, double b); */

/** \defgroup msg_simulation   MSG simulation Functions
 *  \brief This section describes the functions you need to know to
 *  set up a simulation. You should have a look at \ref MSG_examples 
 *  to have an overview of their usage.
 *    \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Simulation functions" --> \endhtmlonly
 */

/********************************* MSG **************************************/

/** \ingroup msg_simulation
 * \brief Initialize some MSG internal data.
 */
void MSG_global_init_args(int *argc, char **argv)
{
  MSG_global_init(argc,argv);
}

/** \ingroup msg_simulation
 * \brief Initialize some MSG internal data.
 */
void MSG_global_init(int *argc, char **argv)
{
	return;

}

/** \ingroup msg_easier_life
 * \brief Traces MSG events in the Paje format.
 */
void MSG_paje_output(const char *filename)
{
	return;
}

/** \defgroup m_channel_management    Understanding channels
 *  \brief This section briefly describes the channel notion of MSG
 *  (#m_channel_t).
 *    \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Channels" --> \endhtmlonly
 * 
 *
 *  For convenience, the simulator provides the notion of channel
 *  that is close to the tag notion in MPI. A channel is not a
 *  socket. It doesn't need to be opened neither closed. It rather
 *  corresponds to the ports opened on the different machines.
 */


/** \ingroup m_channel_management
 * \brief Set the number of channel in the simulation.
 *
 * This function has to be called to fix the number of channel in the
   simulation before creating any host. Indeed, each channel is
   represented by a different mailbox on each #m_host_t. This
   function can then be called only once. This function takes only one
   parameter.
 * \param number the number of channel in the simulation. It has to be >0
 */
MSG_error_t MSG_set_channel_number(int number)
{

  return MSG_OK;
}

/** \ingroup m_channel_management
 * \brief Return the number of channel in the simulation.
 *
 * This function has to be called once the number of channel is fixed. I can't 
   figure out a reason why anyone would like to call this function but nevermind.
 * \return the number of channel in the simulation.
 */
int MSG_get_channel_number(void)
{
	return 0;
}

void __MSG_display_process_status(void)
{
}

/* FIXME: Yeah, I'll do it in a portable maner one day [Mt] */
#include <signal.h>

static void _XBT_CALL inthandler(int ignored)
{
   INFO0("CTRL-C pressed. Displaying status and bailing out");
   __MSG_display_process_status();
   exit(1);
}

/** \ingroup msg_simulation
 * \brief Launch the MSG simulation
 */
MSG_error_t MSG_main(void)
{
  return MSG_OK;
}

/** \ingroup msg_simulation
 * \brief Kill all running process

 * \param reset_PIDs should we reset the PID numbers. A negative
 *   number means no reset and a positive number will be used to set the PID
 *   of the next newly created process.
 */
int MSG_process_killall(int reset_PIDs)
{
  return 0;
}

/** \ingroup msg_simulation
 * \brief Clean the MSG simulation
 */
MSG_error_t MSG_clean(void)
{
  return MSG_OK;
}


/** \ingroup msg_easier_life
 * \brief A clock (in second).
 */
double MSG_get_clock(void) 
{
	return 0.0;
}

