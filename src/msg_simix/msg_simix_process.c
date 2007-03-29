
#include "msg_simix_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** \defgroup m_process_management Management Functions of Agents
 *  \brief This section describes the agent structure of MSG
 *  (#m_process_t) and the functions for managing it.
 *    \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Agents" --> \endhtmlonly
 * 
 *  We need to simulate many independent scheduling decisions, so
 *  the concept of <em>process</em> is at the heart of the
 *  simulator. A process may be defined as a <em>code</em>, with
 *  some <em>private data</em>, executing in a <em>location</em>.
 *  \see m_process_t
 */

/******************************** Process ************************************/
/** \ingroup m_process_management
 * \brief Creates and runs a new #m_process_t.
 *
 * Does exactly the same as #MSG_process_create_with_arguments but without 
   providing standard arguments (\a argc, \a argv, \a start_time, \a kill_time).
 * \sa MSG_process_create_with_arguments
 */
m_process_t MSG_process_create(const char *name,
			       m_process_code_t code, void *data,
			       m_host_t host)
{
  return MSG_process_create_with_arguments(name, code, data, host, -1, NULL);
}

static void MSG_process_cleanup(void *arg)
{
	/* arg is a pointer to a simix process, we can get the msg process with the field data */
	m_process_t proc = ((smx_process_t)arg)->data;
  xbt_fifo_remove(msg_global->process_list, proc);
	SIMIX_process_cleanup(arg);
  free(proc->name);
  proc->name = NULL;
  free(proc->simdata);
  proc->simdata = NULL;
  free(proc);

	return;
}

/** \ingroup m_process_management
 * \brief Creates and runs a new #m_process_t.

 * A constructor for #m_process_t taking four arguments and returning the 
 * corresponding object. The structure (and the corresponding thread) is
 * created, and put in the list of ready process.
 * \param name a name for the object. It is for user-level information
   and can be NULL.
 * \param code is a function describing the behavior of the agent. It
   should then only use functions described in \ref
   m_process_management (to create a new #m_process_t for example),
   in \ref m_host_management (only the read-only functions i.e. whose
   name contains the word get), in \ref m_task_management (to create
   or destroy some #m_task_t for example) and in \ref
   msg_gos_functions (to handle file transfers and task processing).
 * \param data a pointer to any data one may want to attach to the new
   object.  It is for user-level information and can be NULL. It can
   be retrieved with the function \ref MSG_process_get_data.
 * \param host the location where the new agent is executed.
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code
 * \see m_process_t
 * \return The new corresponding object.
 */



m_process_t __MSG_process_create_with_arguments(const char *name,
					      m_process_code_t code, void *data,
					      char * hostname, int argc, char **argv)
{
	m_host_t host = MSG_get_host_by_name(hostname);
	return MSG_process_create_with_arguments(name,code,data,host,argc,argv);
}

m_process_t MSG_process_create_with_arguments(const char *name,
					      m_process_code_t code, void *data,
					      m_host_t host, int argc, char **argv)
{
  simdata_process_t simdata = xbt_new0(s_simdata_process_t,1);
  m_process_t process = xbt_new0(s_m_process_t,1);
  xbt_assert0(((code != NULL) && (host != NULL)), "Invalid parameters");

  /* Simulator Data */
  simdata->PID = msg_global->PID++;
  simdata->host = host;
  simdata->argc = argc;
  simdata->argv = argv;
	simdata->smx_process = SIMIX_process_create_with_arguments(name, (smx_process_code_t)code, (void*)process, host->name, argc, argv, MSG_process_cleanup );

	if (SIMIX_process_self()) {
		simdata->PPID = MSG_process_get_PID(SIMIX_process_self()->data);
	}
	else simdata->PPID = -1;
  simdata->last_errno=MSG_OK;


  /* Process structure */
  process->name = xbt_strdup(name);
  process->simdata = simdata;
  process->data = data;

	xbt_fifo_unshift(msg_global->process_list, process); 

  return process;
}

/** \ingroup m_process_management
 * \param process poor victim
 *
 * This function simply kills a \a process... scarry isn't it ? :)
 */
void MSG_process_kill(m_process_t process)
{

	/*
  int i;
  simdata_process_t p_simdata = process->simdata;
  simdata_host_t h_simdata= p_simdata->host->simdata;
  int _cursor;
  m_process_t proc = NULL;

  DEBUG3("Killing %s(%d) on %s",process->name, p_simdata->PID, p_simdata->host->name);

  for (i=0; i<msg_global->max_channel; i++) {
    if (h_simdata->sleeping[i] == process) {
      h_simdata->sleeping[i] = NULL; 
      break;
    }
  }

	if(p_simdata->waiting_task) {    
    xbt_dynar_foreach(p_simdata->waiting_task->simdata->sleeping,_cursor,proc) {                                                                             
      if(proc==process)
  xbt_dynar_remove_at(p_simdata->waiting_task->simdata->sleeping,_cursor,&proc);                                                                             
    }  
    if(p_simdata->waiting_task->simdata->compute)
      surf_workstation_resource->common_public->
  action_free(p_simdata->waiting_task->simdata->compute);
    else if (p_simdata->waiting_task->simdata->comm) {
      surf_workstation_resource->common_public->
  action_change_state(p_simdata->waiting_task->simdata->comm,SURF_ACTION_FAILED);                                                                            
      surf_workstation_resource->common_public->
  action_free(p_simdata->waiting_task->simdata->comm);                                                                                                       
    } else {
      xbt_die("UNKNOWN STATUS. Please report this bug.");                                                                                                    
    }  
  }    

  if ((i==msg_global->max_channel) && (process!=MSG_process_self()) &&                                                                                       
      (!p_simdata->waiting_task)) {
    xbt_die("UNKNOWN STATUS. Please report this bug.");                                                                                                      
  }
*/
  xbt_fifo_remove(msg_global->process_list,process);
	SIMIX_process_kill(process->simdata->smx_process);

	return;
}

/** \ingroup m_process_management
 * \brief Migrates an agent to another location.
 *
 * This functions checks whether \a process and \a host are valid pointers
   and change the value of the #m_host_t on which \a process is running.
 */
MSG_error_t MSG_process_change_host(m_process_t process, m_host_t host)
{
	xbt_die("MSG_process_change_host - not implemented yet - maybe useless function");
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Return the user data of a #m_process_t.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return the user data associated to \a process if it is possible.
 */
void *MSG_process_get_data(m_process_t process)
{
  xbt_assert0((process != NULL), "Invalid parameters");

  return (process->data);
}

/** \ingroup m_process_management
 * \brief Set the user data of a #m_process_t.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and set the user data associated to \a process if it is possible.
 */
MSG_error_t MSG_process_set_data(m_process_t process,void *data)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  xbt_assert0((process->data == NULL), "Data already set");
  
  process->data = data;
   
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Return the location on which an agent is running.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return the m_host_t corresponding to the location on which \a 
   process is running.
 */
m_host_t MSG_process_get_host(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (((simdata_process_t) process->simdata)->host);
}

/** \ingroup m_process_management
 *
 * \brief Return a #m_process_t given its PID.
 *
 * This functions search in the list of all the created m_process_t for a m_process_t 
   whose PID is equal to \a PID. If no host is found, \c NULL is returned. 
   Note that the PID are uniq in the whole simulation, not only on a given host.
 */
m_process_t MSG_process_from_PID(int PID)
{
  xbt_fifo_item_t i = NULL;
  m_process_t process = NULL;

  xbt_fifo_foreach(msg_global->process_list,i,process,m_process_t) {
    if(MSG_process_get_PID(process) == PID) return process;
  }
  return NULL;
}

/** \ingroup m_process_management
 * \brief Returns the process ID of \a process.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return its PID.
 */
int MSG_process_get_PID(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (((simdata_process_t) process->simdata)->PID);
}

/** \ingroup m_process_management
 * \brief Returns the process ID of the parent of \a process.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return its PID. Returns -1 if the agent has not been created by 
   another agent.
 */
int MSG_process_get_PPID(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (((simdata_process_t) process->simdata)->PPID);
}

/** \ingroup m_process_management
 * \brief Return the name of an agent.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return its name.
 */
const char *MSG_process_get_name(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (process->name);
}

/** \ingroup m_process_management
 * \brief Return the PID of the current agent.
 *
 * This functions returns the PID of the currently running #m_process_t.
 */
int MSG_process_self_PID(void)
{
  return (MSG_process_get_PID(MSG_process_self()));
}

/** \ingroup m_process_management
 * \brief Return the PPID of the current agent.
 *
 * This functions returns the PID of the parent of the currently
 * running #m_process_t.
 */
int MSG_process_self_PPID(void)
{
  return (MSG_process_get_PPID(MSG_process_self()));
}

/** \ingroup m_process_management
 * \brief Return the current agent.
 *
 * This functions returns the currently running #m_process_t.
 */
m_process_t MSG_process_self(void)
{
	smx_process_t proc = SIMIX_process_self();
	if (proc != NULL) {
		return (m_process_t)proc->data;
		}
	else { 
		return NULL;
	}

}

/** \ingroup m_process_management
 * \brief Suspend the process.
 *
 * This functions suspend the process by suspending the task on which
 * it was waiting for the completion.
 */
MSG_error_t MSG_process_suspend(m_process_t process)
{
	xbt_die("not implemented yet");
	return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Resume a suspended process.
 *
 * This functions resume a suspended process by resuming the task on
 * which it was waiting for the completion.
 */
MSG_error_t MSG_process_resume(m_process_t process)
{
	xbt_die("not implemented yet");
	MSG_RETURN(MSG_OK);
}

/** \ingroup m_process_management
 * \brief Returns true if the process is suspended .
 *
 * This checks whether a process is suspended or not by inspecting the
 * task on which it was waiting for the completion.
 */
int MSG_process_is_suspended(m_process_t process)
{
	xbt_die("not implemented yet");
	return 0;
}

int __MSG_process_block(double max_duration, const char *info)
{
    return 1;
}

MSG_error_t __MSG_process_unblock(m_process_t process)
{
    MSG_RETURN(MSG_OK);
}

int __MSG_process_isBlocked(m_process_t process)
{
	return 0;
}
