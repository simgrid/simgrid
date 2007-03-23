#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h" /* ex_backtrace_display */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_kernel, msg,        "Logging specific to MSG (kernel)");    

MSG_Global_t msg_global = NULL;


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
  if (!msg_global) {
    SIMIX_global_init(argc, argv);
     
    msg_global = xbt_new0(s_MSG_Global_t,1);

    msg_global->host = xbt_fifo_new();
    msg_global->process_list = xbt_fifo_new();
    msg_global->max_channel = 0;
    msg_global->PID = 1;
  }
	return;

}

/** \ingroup msg_easier_life
 * \brief Traces MSG events in the Paje format.
 */

void MSG_paje_output(const char *filename)
{
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
  xbt_assert0((msg_global) && (msg_global->max_channel == 0), "Channel number already set!");

  msg_global->max_channel = number;

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
  xbt_assert0((msg_global)&&(msg_global->max_channel != 0), "Channel number not set yet!");

  return msg_global->max_channel;
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
	smx_cond_t cond = NULL;
	smx_action_t smx_action;
	xbt_fifo_t actions_done = xbt_fifo_new();
	xbt_fifo_t actions_failed = xbt_fifo_new();

	/* Prepare to display some more info when dying on Ctrl-C pressing */
	signal(SIGINT,inthandler);

	/* Clean IO before the run */
	fflush(stdout);
	fflush(stderr);

	//surf_solve(); /* Takes traces into account. Returns 0.0 */
	/* xbt_fifo_size(msg_global->process_to_run) */

	while (SIMIX_solve(actions_done, actions_failed) != -1.0) {

		while ( (smx_action = xbt_fifo_pop(actions_failed)) ) {

			xbt_fifo_item_t _cursor;

			DEBUG1("** %s failed **",smx_action->name);
			xbt_fifo_foreach(smx_action->cond_list,_cursor,cond,smx_cond_t) {
				SIMIX_cond_broadcast(cond);
				/* remove conditional from action */
				xbt_fifo_remove(smx_action->cond_list,cond);
			}
		}

		while ( (smx_action = xbt_fifo_pop(actions_done)) ) {
			xbt_fifo_item_t _cursor;

			DEBUG1("** %s done **",smx_action->name);
			xbt_fifo_foreach(smx_action->cond_list,_cursor,cond,smx_cond_t) {
				SIMIX_cond_broadcast(cond);
				/* remove conditional from action */
				xbt_fifo_remove(smx_action->cond_list,cond);
			}
		}
	}
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
	xbt_die("not implemented yet");
  return 0;
}

/** \ingroup msg_simulation
 * \brief Clean the MSG simulation
 */
MSG_error_t MSG_clean(void)
{
  xbt_fifo_item_t i = NULL;
  m_host_t h = NULL;
  m_process_t p = NULL;


  while((p=xbt_fifo_pop(msg_global->process_list))) {
    MSG_process_kill(p);
  }

  xbt_fifo_foreach(msg_global->host,i,h,m_host_t) {
    __MSG_host_destroy(h);
  }
  xbt_fifo_free(msg_global->host);
  xbt_fifo_free(msg_global->process_list);

  free(msg_global);
	SIMIX_clean();

  return MSG_OK;
}


/** \ingroup msg_easier_life
 * \brief A clock (in second).
 */
double MSG_get_clock(void) 
{
	return SIMIX_get_clock();
}

