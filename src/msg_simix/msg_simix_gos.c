#include "msg_simix_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** \defgroup msg_gos_functions MSG Operating System Functions
 *  \brief This section describes the functions that can be used
 *  by an agent for handling some task.
 */

static MSG_error_t __MSG_task_get_with_time_out_from_host(m_task_t * task,
							m_channel_t channel,
							double max_duration,
							m_host_t host)
{

	xbt_die("not implemented yet");
	MSG_RETURN(MSG_OK);
}

/** \ingroup msg_gos_functions
 * \brief Listen on a channel and wait for receiving a task.
 *
 * It takes two parameters.
 * \param task a memory location for storing a #m_task_t. It will
   hold a task when this function will return. Thus \a task should not
   be equal to \c NULL and \a *task should be equal to \c NULL. If one of
   those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
 * if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */
MSG_error_t MSG_task_get(m_task_t * task,
			 m_channel_t channel)
{
  return MSG_task_get_with_time_out(task, channel, -1);
}

/** \ingroup msg_gos_functions
 * \brief Listen on a channel and wait for receiving a task with a timeout.
 *
 * It takes three parameters.
 * \param task a memory location for storing a #m_task_t. It will
   hold a task when this function will return. Thus \a task should not
   be equal to \c NULL and \a *task should be equal to \c NULL. If one of
   those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \param max_duration the maximum time to wait for a task before giving
    up. In such a case, #MSG_TRANSFER_FAILURE will be returned, \a task 
    will not be modified and will still be
    equal to \c NULL when returning. 
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
   if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */
MSG_error_t MSG_task_get_with_time_out(m_task_t * task,
				       m_channel_t channel,
				       double max_duration)
{
  return __MSG_task_get_with_time_out_from_host(task, channel, max_duration, NULL);
}

/** \ingroup msg_gos_functions
 * \brief Listen on \a channel and waits for receiving a task from \a host.
 *
 * It takes three parameters.
 * \param task a memory location for storing a #m_task_t. It will
   hold a task when this function will return. Thus \a task should not
   be equal to \c NULL and \a *task should be equal to \c NULL. If one of
   those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \param host the host that is to be watched.
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
   if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */
MSG_error_t MSG_task_get_from_host(m_task_t * task, int channel, 
				   m_host_t host)
{
  return __MSG_task_get_with_time_out_from_host(task, channel, -1, host);
}

/** \ingroup msg_gos_functions
 * \brief Test whether there is a pending communication on a channel.
 *
 * It takes one parameter.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \return 1 if there is a pending communication and 0 otherwise
 */
int MSG_task_Iprobe(m_channel_t channel)
{
	xbt_die("not implemented yet");
	return 0;
}

/** \ingroup msg_gos_functions
 * \brief Test whether there is a pending communication on a channel, and who sent it.
 *
 * It takes one parameter.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \return -1 if there is no pending communication and the PID of the process who sent it otherwise
 */
int MSG_task_probe_from(m_channel_t channel)
{
	xbt_die("not implemented yet");
	return 0;
}

/** \ingroup msg_gos_functions
 * \brief Wait for at most \a max_duration second for a task reception
   on \a channel. *\a PID is updated with the PID of the first process
   that triggered this event if any.
 *
 * It takes three parameters:
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal.
   number of channels fixed with MSG_set_channel_number().
 * \param PID a memory location for storing an int.
 * \param max_duration the maximum time to wait for a task before
    giving up. In the case of a reception, *\a PID will be updated
    with the PID of the first process to send a task.
 * \return #MSG_HOST_FAILURE if the host is shut down in the meantime
   and #MSG_OK otherwise.
 */
MSG_error_t MSG_channel_select_from(m_channel_t channel, double max_duration,
				    int *PID)
{
	xbt_die("not implemented yet");
	MSG_RETURN(MSG_OK);
}


/** \ingroup msg_gos_functions

 * \brief Return the number of tasks waiting to be received on a \a
   channel and sent by \a host.
 *
 * It takes two parameters.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \param host the host that is to be watched.
 * \return the number of tasks waiting to be received on \a channel
   and sent by \a host.
 */
int MSG_task_probe_from_host(int channel, m_host_t host)
{
	xbt_die("not implemented yet");
	return 0;
}

/** \ingroup msg_gos_functions \brief Put a task on a channel of an
 * host (with a timeout on the waiting of the destination host) and
 * waits for the end of the transmission.
 *
 * This function is used for describing the behavior of an agent. It
 * takes four parameter.
 * \param task a #m_task_t to send on another location. This task
   will not be usable anymore when the function will return. There is
   no automatic task duplication and you have to save your parameters
   before calling this function. Tasks are unique and once it has been
   sent to another location, you should not access it anymore. You do
   not need to call MSG_task_destroy() but to avoid using, as an
   effect of inattention, this task anymore, you definitely should
   renitialize it with #MSG_TASK_UNINITIALIZED. Note that this task
   can be transfered iff it has been correctly created with
   MSG_task_create().
 * \param dest the destination of the message
 * \param channel the channel on which the agent should put this
   task. This value has to be >=0 and < than the maximal number of
   channels fixed with MSG_set_channel_number().
 * \param max_duration the maximum time to wait for a task before giving
    up. In such a case, #MSG_TRANSFER_FAILURE will be returned, \a task 
    will not be modified 
 * \return #MSG_FATAL if \a task is not properly initialized and
   #MSG_OK otherwise. Returns #MSG_HOST_FAILURE if the host on which
   this function was called was shut down. Returns
   #MSG_TRANSFER_FAILURE if the transfer could not be properly done
   (network failure, dest failure, timeout...)
 */
MSG_error_t MSG_task_put_with_timeout(m_task_t task, m_host_t dest, 
				      m_channel_t channel, double max_duration)
{
	xbt_die("not implemented yet");
	MSG_RETURN(MSG_OK);
}
/** \ingroup msg_gos_functions
 * \brief Put a task on a channel of an host and waits for the end of the
 * transmission.
 *
 * This function is used for describing the behavior of an agent. It
 * takes three parameter.
 * \param task a #m_task_t to send on another location. This task
   will not be usable anymore when the function will return. There is
   no automatic task duplication and you have to save your parameters
   before calling this function. Tasks are unique and once it has been
   sent to another location, you should not access it anymore. You do
   not need to call MSG_task_destroy() but to avoid using, as an
   effect of inattention, this task anymore, you definitely should
   renitialize it with #MSG_TASK_UNINITIALIZED. Note that this task
   can be transfered iff it has been correctly created with
   MSG_task_create().
 * \param dest the destination of the message
 * \param channel the channel on which the agent should put this
   task. This value has to be >=0 and < than the maximal number of
   channels fixed with MSG_set_channel_number().
 * \return #MSG_FATAL if \a task is not properly initialized and
 * #MSG_OK otherwise. Returns #MSG_HOST_FAILURE if the host on which
 * this function was called was shut down. Returns
 * #MSG_TRANSFER_FAILURE if the transfer could not be properly done
 * (network failure, dest failure)
 */
MSG_error_t MSG_task_put(m_task_t task,
			 m_host_t dest, m_channel_t channel)
{
  return MSG_task_put_with_timeout(task, dest, channel, -1.0);
}

/** \ingroup msg_gos_functions
 * \brief Does exactly the same as MSG_task_put but with a bounded transmition 
 * rate.
 *
 * \sa MSG_task_put
 */
MSG_error_t MSG_task_put_bounded(m_task_t task,
				 m_host_t dest, m_channel_t channel,
				 double max_rate)
{
  MSG_error_t res = MSG_OK;
  task->simdata->rate=max_rate;
  res = MSG_task_put(task, dest, channel);
  return(res);
}

/** \ingroup msg_gos_functions
 * \brief Executes a task and waits for its termination.
 *
 * This function is used for describing the behavior of an agent. It
 * takes only one parameter.
 * \param task a #m_task_t to execute on the location on which the
   agent is running.
 * \return #MSG_FATAL if \a task is not properly initialized and
 * #MSG_OK otherwise.
 */
MSG_error_t MSG_task_execute(m_task_t task)
{
	simdata_task_t simdata = NULL;

  CHECK_HOST();

  simdata = task->simdata;
  xbt_assert0((!simdata->compute)&&(task->simdata->using==1),
	      "This taks is executed somewhere else. Go fix your code!");
  simdata->using++;
	SIMIX_mutex_lock(simdata->mutex);
  simdata->compute = SIMIX_action_execute(SIMIX_host_self(), task->name, simdata->computation_amount);
	SIMIX_action_set_priority(simdata->compute, simdata->priority);

	SIMIX_register_action_to_condition(simdata->compute, simdata->cond);
	SIMIX_register_condition_to_action(simdata->compute, simdata->cond);
	
	SIMIX_cond_wait(simdata->cond, simdata->mutex);

	SIMIX_mutex_unlock(simdata->mutex);
  simdata->using--;

//	MSG_RETURN(MSG_OK);
	return MSG_OK;
}

void __MSG_task_execute(m_process_t process, m_task_t task)
{

}

MSG_error_t __MSG_wait_for_computation(m_process_t process, m_task_t task)
{
	MSG_RETURN(MSG_OK);
}
/** \ingroup m_task_management
 * \brief Creates a new #m_task_t (a parallel one....).
 *
 * A constructor for #m_task_t taking six arguments and returning the 
   corresponding object.
 * \param name a name for the object. It is for user-level information
   and can be NULL.
 * \param host_nb the number of hosts implied in the parallel task.
 * \param host_list an array of \p host_nb m_host_t.
 * \param computation_amount an array of \p host_nb
   doubles. computation_amount[i] is the total number of operations
   that have to be performed on host_list[i].
 * \param communication_amount an array of \p host_nb* \p host_nb doubles.
 * \param data a pointer to any data may want to attach to the new
   object.  It is for user-level information and can be NULL. It can
   be retrieved with the function \ref MSG_task_get_data.
 * \see m_task_t
 * \return The new corresponding object.
 */
m_task_t MSG_parallel_task_create(const char *name, 
				  int host_nb,
				  const m_host_t *host_list,
				  double *computation_amount,
				  double *communication_amount,
				  void *data)
{
  m_task_t task = xbt_new0(s_m_task_t,1);
	xbt_die("not implemented yet");
  return task;
}


static void __MSG_parallel_task_execute(m_process_t process, m_task_t task)
{
	return;
}

MSG_error_t MSG_parallel_task_execute(m_task_t task)
{

	xbt_die("not implemented yet");
  return MSG_OK;  
}


/** \ingroup msg_gos_functions
 * \brief Sleep for the specified number of seconds
 *
 * Makes the current process sleep until \a time seconds have elapsed.
 *
 * \param nb_sec a number of second
 */
MSG_error_t MSG_process_sleep(double nb_sec)
{
	smx_action_t act_sleep;
	m_process_t proc = MSG_process_self();
	smx_mutex_t mutex;
	smx_cond_t cond;
	/* create action to sleep */
	act_sleep = SIMIX_action_sleep(SIMIX_process_get_host(proc->simdata->smx_process),nb_sec);
	
	mutex = SIMIX_mutex_init();
	SIMIX_mutex_lock(mutex);
	/* create conditional and register action to it */
	cond = SIMIX_cond_init();

	SIMIX_register_condition_to_action(act_sleep, cond);
	SIMIX_register_action_to_condition(act_sleep, cond);
	SIMIX_cond_wait(cond,mutex);
	SIMIX_mutex_unlock(mutex);

	/* remove variables */
	SIMIX_action_destroy(act_sleep);
	SIMIX_cond_destroy(cond);
	SIMIX_mutex_destroy(mutex);

	MSG_RETURN(MSG_OK);
}

/** \ingroup msg_gos_functions
 * \brief Return the number of MSG tasks currently running on
 * the host of the current running process.
 */
static int MSG_get_msgload(void) 
{
	xbt_die("not implemented yet");
	return 0;
}

/** \ingroup msg_gos_functions
 *
 * \brief Return the last value returned by a MSG function (except
 * MSG_get_errno...).
 */
MSG_error_t MSG_get_errno(void)
{
  return PROCESS_GET_ERRNO();
}
