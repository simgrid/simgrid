/* smx_user.c - public interface to simix                                   */

/* Copyright (c) 2010, 2011. Da SimGrid team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef _SVID_SOURCE
#  define _SVID_SOURCE    /* strdup() */
#endif
#ifndef _ISOC99_SOURCE
#  define _ISOC99_SOURCE  /* isfinite() */
#endif
#ifndef _ISO_C99_SOURCE
#  define _ISO_C99_SOURCE /* isfinite() */
#endif
#include <math.h>         /* isfinite() */

#include "smx_private.h"
#include "mc/mc.h"
#include "xbt/ex.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix);

static const char* simcall_names[] = {
#undef SIMCALL_ENUM_ELEMENT
#define SIMCALL_ENUM_ELEMENT(x) #x /* generate strings from the enumeration values */
SIMCALL_LIST
#undef SIMCALL_ENUM_ELEMENT
};

/**
 * \brief Returns a host given its name.
 *
 * \param name The name of the host to get
 * \return The corresponding host
 */
smx_host_t simcall_host_get_by_name(const char *name)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_GET_BY_NAME;
  simcall->host_get_by_name.name = name;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_get_by_name.result;
}

/**
 * \brief Returns the name of a host.
 *
 * \param host A SIMIX host
 * \return The name of this host
 */
const char* simcall_host_get_name(smx_host_t host)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_GET_NAME;
  simcall->host_get_name.host = host;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_get_name.result;
}

/**
 * \brief Returns a dict of the properties assigned to a host.
 *
 * \param host A host
 * \return The properties of this host
 */
xbt_dict_t simcall_host_get_properties(smx_host_t host)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_GET_PROPERTIES;
  simcall->host_get_properties.host = host;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_get_properties.result;
}

/**
 * \brief Returns the speed of the processor.
 *
 * The speed returned does not take into account the current load on the machine.
 * \param host A SIMIX host
 * \return The speed of this host (in Mflop/s)
 */
double simcall_host_get_speed(smx_host_t host)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_GET_SPEED;
  simcall->host_get_speed.host = host;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_get_speed.result;
}

/**
 * \brief Returns the available speed of the processor.
 *
 * \return Speed currently available (in Mflop/s)
 */
double simcall_host_get_available_speed(smx_host_t host)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_GET_AVAILABLE_SPEED;
  simcall->host_get_available_speed.host = host;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_get_available_speed.result;
}

/**
 * \brief Returns the state of a host.
 *
 * Two states are possible: 1 if the host is active or 0 if it has crashed.
 * \param host A SIMIX host
 * \return 1 if the host is available, 0 otherwise
 */
int simcall_host_get_state(smx_host_t host)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_GET_STATE;
  simcall->host_get_state.host = host;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_get_state.result;
}

/**
 * \brief Returns the user data associated to a host.
 *
 * \param host SIMIX host
 * \return the user data of this host
 */
void* simcall_host_get_data(smx_host_t host)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_GET_DATA;
  simcall->host_get_data.host = host;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_get_data.result;
}

/**
 * \brief Sets the user data associated to a host.
 *
 * The host must not have previous user data associated to it.
 * \param A host SIMIX host
 * \param data The user data to set
 */
void simcall_host_set_data(smx_host_t host, void *data)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_SET_DATA;
  simcall->host_set_data.host = host;
  simcall->host_set_data.data = data;
  SIMIX_simcall_push(simcall->issuer);
}

/** \brief Creates an action that executes some computation of an host.
 *
 * This function creates a SURF action and allocates the data necessary
 * to create the SIMIX action. It can raise a host_error exception if the host crashed.
 *
 * \param name Name of the execution action to create
 * \param host SIMIX host where the action will be executed
 * \param amount Computation amount (in bytes)
 * \return A new SIMIX execution action
 */
smx_action_t simcall_host_execute(const char *name, smx_host_t host,
                                    double computation_amount,
                                    double priority)
{
  /* checking for infinite values */
  xbt_assert(isfinite(computation_amount), "computation_amount is not finite!");
  xbt_assert(isfinite(priority), "priority is not finite!");
  
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_EXECUTE;
  simcall->host_execute.name = name;
  simcall->host_execute.host = host;
  simcall->host_execute.computation_amount = computation_amount;
  simcall->host_execute.priority = priority;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_execute.result;
}

/** \brief Creates an action that may involve parallel computation on
 * several hosts and communication between them.
 *
 * \param name Name of the execution action to create
 * \param host_nb Number of hosts where the action will be executed
 * \param host_list Array (of size host_nb) of hosts where the action will be executed
 * \param computation_amount Array (of size host_nb) of computation amount of hosts (in bytes)
 * \param communication_amount Array (of size host_nb * host_nb) representing the communication
 * amount between each pair of hosts
 * \param amount the SURF action amount
 * \param rate the SURF action rate
 * \return A new SIMIX execution action
 */
smx_action_t simcall_host_parallel_execute(const char *name,
                                         int host_nb,
                                         smx_host_t *host_list,
                                         double *computation_amount,
                                         double *communication_amount,
                                         double amount,
                                         double rate)
{
  int i,j;
  /* checking for infinite values */
  for (i = 0 ; i < host_nb ; ++i) {
     xbt_assert(isfinite(computation_amount[i]), "computation_amount[%d] is not finite!", i);
     for (j = 0 ; j < host_nb ; ++j) {
        xbt_assert(isfinite(communication_amount[i + host_nb * j]), 
	           "communication_amount[%d+%d*%d] is not finite!", i, host_nb, j);
     }   
  }   
 
  xbt_assert(isfinite(amount), "amount is not finite!");
  xbt_assert(isfinite(rate), "rate is not finite!");
  
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_PARALLEL_EXECUTE;
  simcall->host_parallel_execute.name = name;
  simcall->host_parallel_execute.host_nb = host_nb;
  simcall->host_parallel_execute.host_list = host_list;
  simcall->host_parallel_execute.computation_amount = computation_amount;
  simcall->host_parallel_execute.communication_amount = communication_amount;
  simcall->host_parallel_execute.amount = amount;
  simcall->host_parallel_execute.rate = rate;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_parallel_execute.result;
}

/**
 * \brief Destroys an execution action.
 *
 * Destroys an action, freing its memory. This function cannot be called if there are a conditional waiting for it.
 * \param action The execution action to destroy
 */
void simcall_host_execution_destroy(smx_action_t execution)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_EXECUTION_DESTROY;
  simcall->host_execution_destroy.execution = execution;
  SIMIX_simcall_push(simcall->issuer);
}

/**
 * \brief Cancels an execution action.
 *
 * This functions stops the execution. It calls a surf function.
 * \param action The execution action to cancel
 */
void simcall_host_execution_cancel(smx_action_t execution)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_EXECUTION_CANCEL;
  simcall->host_execution_cancel.execution = execution;
  SIMIX_simcall_push(simcall->issuer);
}

/**
 * \brief Returns how much of an execution action remains to be done.
 *
 * \param Action The execution action
 * \return The remaining amount
 */
double simcall_host_execution_get_remains(smx_action_t execution)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_EXECUTION_GET_REMAINS;
  simcall->host_execution_get_remains.execution = execution;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_execution_get_remains.result;
}

/**
 * \brief Returns the state of an execution action.
 *
 * \param execution The execution action
 * \return The state
 */
e_smx_state_t simcall_host_execution_get_state(smx_action_t execution)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_EXECUTION_GET_STATE;
  simcall->host_execution_get_state.execution = execution;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_execution_get_state.result;
}

/**
 * \brief Changes the priority of an execution action.
 *
 * This functions changes the priority only. It calls a surf function.
 * \param execution The execution action
 * \param priority The new priority
 */
void simcall_host_execution_set_priority(smx_action_t execution, double priority)
{
  /* checking for infinite values */
  xbt_assert(isfinite(priority), "priority is not finite!");
  
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_EXECUTION_SET_PRIORITY;
  simcall->host_execution_set_priority.execution = execution;
  simcall->host_execution_set_priority.priority = priority;
  SIMIX_simcall_push(simcall->issuer);
}

/**
 * \brief Waits for the completion of an execution action and destroy it.
 *
 * \param execution The execution action
 */
e_smx_state_t simcall_host_execution_wait(smx_action_t execution)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_HOST_EXECUTION_WAIT;
  simcall->host_execution_wait.execution = execution;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->host_execution_wait.result;
}

/**
 * \brief Creates and runs a new SIMIX process.
 *
 * The structure and the corresponding thread are created and put in the list of ready processes.
 *
 * \param process the process created will be stored in this pointer
 * \param name a name for the process. It is for user-level information and can be NULL.
 * \param code the main function of the process
 * \param data a pointer to any data one may want to attach to the new object. It is for user-level information and can be NULL.
 * It can be retrieved with the function \ref simcall_process_get_data.
 * \param hostname name of the host where the new agent is executed.
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code
 * \param properties the properties of the process
 */
void simcall_process_create(smx_process_t *process, const char *name,
                              xbt_main_func_t code,
                              void *data,
                              const char *hostname,
                              int argc, char **argv,
                              xbt_dict_t properties)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_CREATE;
  simcall->process_create.process = process;
  simcall->process_create.name = name;
  simcall->process_create.code = code;
  simcall->process_create.data = data;
  simcall->process_create.hostname = hostname;
  simcall->process_create.argc = argc;
  simcall->process_create.argv = argv;
  simcall->process_create.properties = properties;
  SIMIX_simcall_push(simcall->issuer);
}

/** \brief Kills a SIMIX process.
 *
 * This function simply kills a  process.
 *
 * \param process poor victim
 */
void simcall_process_kill(smx_process_t process)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_KILL;
  simcall->process_kill.process = process;
  SIMIX_simcall_push(simcall->issuer);
}

/** \brief Kills all SIMIX processes.
 */
void simcall_process_killall(void)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_KILLALL;
  SIMIX_simcall_push(simcall->issuer);
}

/** \brief Cleans up a SIMIX process.
 * \param process poor victim (must have already been killed)
 */
void simcall_process_cleanup(smx_process_t process)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_CLEANUP;
  simcall->process_cleanup.process = process;
  SIMIX_simcall_push(simcall->issuer);
}

/**
 * \brief Migrates an agent to another location.
 *
 * This function changes the value of the host on which \a process is running.
 *
 * \param process the process to migrate
 * \param source name of the previous host
 * \param dest name of the new host
 */
void simcall_process_change_host(smx_process_t process, smx_host_t dest)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_CHANGE_HOST;
  simcall->process_change_host.process = process;
  simcall->process_change_host.dest = dest;
  SIMIX_simcall_push(simcall->issuer);
}

/**
 * \brief Suspends a process.
 *
 * This function suspends the process by suspending the action
 * it was waiting for completion.
 *
 * \param process a SIMIX process
 */
void simcall_process_suspend(smx_process_t process)
{
  xbt_assert(process, "Invalid parameters");

  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_SUSPEND;
  simcall->process_suspend.process = process;
  SIMIX_simcall_push(simcall->issuer);
}

/**
 * \brief Resumes a suspended process.
 *
 * This function resumes a suspended process by resuming the action
 * it was waiting for completion.
 *
 * \param process a SIMIX process
 */
void simcall_process_resume(smx_process_t process)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_RESUME;
  simcall->process_resume.process = process;
  SIMIX_simcall_push(simcall->issuer);
}

/**
 * \brief Returns the amount of SIMIX processes in the system
 *
 * Maestro internal process is not counted, only user code processes are
 */
int simcall_process_count(void)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_COUNT;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->process_count.result;
}

/**
 * \brief Return the user data of a #smx_process_t.
 * \param process a SIMIX process
 * \return the user data of this process
 */
void* simcall_process_get_data(smx_process_t process)
{
  if (process == SIMIX_process_self()) {
    /* avoid a simcall if this function is called by the process itself */
    return SIMIX_process_get_data(process);
  }

  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_GET_DATA;
  simcall->process_get_data.process = process;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->process_get_data.result;
}

/**
 * \brief Set the user data of a #m_process_t.
 *
 * This functions checks whether \a process is a valid pointer or not and set the user data associated to \a process if it is possible.
 * \param process SIMIX process
 * \param data User data
 */
void simcall_process_set_data(smx_process_t process, void *data)
{
  if (process == SIMIX_process_self()) {
    /* avoid a simcall if this function is called by the process itself */
    SIMIX_process_self_set_data(process, data);
  }
  else {

    smx_simcall_t simcall = SIMIX_simcall_mine();

    simcall->call = SIMCALL_PROCESS_SET_DATA;
    simcall->process_set_data.process = process;
    simcall->process_set_data.data = data;
    SIMIX_simcall_push(simcall->issuer);
  }
}

/**
 * \brief Return the location on which an agent is running.
 *
 * This functions checks whether \a process is a valid pointer or not and return the m_host_t corresponding to the location on which \a process is running.
 * \param process SIMIX process
 * \return SIMIX host
 */
smx_host_t simcall_process_get_host(smx_process_t process)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_GET_HOST;
  simcall->process_get_host.process = process;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->process_get_host.result;
}

/**
 * \brief Return the name of an agent.
 *
 * This functions checks whether \a process is a valid pointer or not and return its name.
 * \param process SIMIX process
 * \return The process name
 */
const char* simcall_process_get_name(smx_process_t process)
{
  if (process == SIMIX_process_self()) {
    /* avoid a simcall if this function is called by the process itself */
    return process->name;
  }

  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_GET_NAME;
  simcall->process_get_name.process = process;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->process_get_name.result;
}

/**
 * \brief Returns true if the process is suspended .
 *
 * This checks whether a process is suspended or not by inspecting the task on which it was waiting for the completion.
 * \param process SIMIX process
 * \return 1, if the process is suspended, else 0.
 */
int simcall_process_is_suspended(smx_process_t process)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_IS_SUSPENDED;
  simcall->process_is_suspended.process = process;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->process_is_suspended.result;
}

/** \ingroup m_process_management
 * \brief Return the properties
 *
 * This functions returns the properties associated with this process
 */
xbt_dict_t simcall_process_get_properties(smx_process_t process)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_GET_PROPERTIES;
  simcall->process_get_properties.process = process;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->process_get_properties.result;
}

/** \brief Creates a new sleep SIMIX action.
 *
 * This function creates a SURF action and allocates the data necessary
 * to create the SIMIX action. It can raise a host_error exception if the
 * host crashed. The default SIMIX name of the action is "sleep".
 *
 * 	\param duration Time duration of the sleep.
 * 	\return A result telling whether the sleep was successful
 */
e_smx_state_t simcall_process_sleep(double duration)
{
  /* checking for infinite values */
  xbt_assert(isfinite(duration), "duration is not finite!");
  
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_PROCESS_SLEEP;
  simcall->process_sleep.duration = duration;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->process_sleep.result;
}

/**
 *  \brief Creates a new rendez-vous point
 *  \param name The name of the rendez-vous point
 *  \return The created rendez-vous point
 */
smx_rdv_t simcall_rdv_create(const char *name)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_RDV_CREATE;
  simcall->rdv_create.name = name;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->rdv_create.result;
}


/**
 *  \brief Destroy a rendez-vous point
 *  \param name The rendez-vous point to destroy
 */
void simcall_rdv_destroy(smx_rdv_t rdv)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_RDV_DESTROY;
  simcall->rdv_destroy.rdv = rdv;

  SIMIX_simcall_push(simcall->issuer);
}

smx_rdv_t simcall_rdv_get_by_name(const char *name)
{
  xbt_assert(name != NULL, "Invalid parameter for simcall_rdv_get_by_name (name is NULL)");

  /* FIXME: this is a horrible lost of performance, so we hack it out by
   * skipping the simcall (for now). It works in parallel, it won't work on
   * distributed but probably we will change MSG for that. */

  /*
  smx_simcall_t simcall = simcall_mine();
  simcall->call = SIMCALL_RDV_GEY_BY_NAME;
  simcall->rdv_get_by_name.name = name;
  SIMIX_simcall_push(simcall->issuer);
  return simcall->rdv_get_by_name.result;*/

  return SIMIX_rdv_get_by_name(name);
}

/**
 *  \brief Counts the number of communication actions of a given host pending
 *         on a rendez-vous point.
 *  \param rdv The rendez-vous point
 *  \param host The host to be counted
 *  \return The number of comm actions pending in the rdv
 */
int simcall_rdv_comm_count_by_host(smx_rdv_t rdv, smx_host_t host)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_RDV_COMM_COUNT_BY_HOST;
  simcall->rdv_comm_count_by_host.rdv = rdv;
  simcall->rdv_comm_count_by_host.host = host;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->rdv_comm_count_by_host.result;
}

/**
 *  \brief returns the communication at the head of the rendez-vous
 *  \param rdv The rendez-vous point
 *  \return The communication or NULL if empty
 */
smx_action_t simcall_rdv_get_head(smx_rdv_t rdv)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_RDV_GET_HEAD;
  simcall->rdv_get_head.rdv = rdv;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->rdv_get_head.result;
}

void simcall_comm_send(smx_rdv_t rdv, double task_size, double rate,
                         void *src_buff, size_t src_buff_size,
                         int (*match_fun)(void *, void *), void *data,
                         double timeout)
{
  /* checking for infinite values */
  xbt_assert(isfinite(task_size), "task_size is not finite!");
  xbt_assert(isfinite(rate), "rate is not finite!");
  xbt_assert(isfinite(timeout), "timeout is not finite!");
  
  xbt_assert(rdv, "No rendez-vous point defined for send");

  if (MC_IS_ENABLED) {
    /* the model-checker wants two separate simcalls */
    smx_action_t comm = simcall_comm_isend(rdv, task_size, rate,
        src_buff, src_buff_size, match_fun, NULL, data, 0);
    simcall_comm_wait(comm, timeout);
  }
  else {
    smx_simcall_t simcall = SIMIX_simcall_mine();

    simcall->call = SIMCALL_COMM_SEND;
    simcall->comm_send.rdv = rdv;
    simcall->comm_send.task_size = task_size;
    simcall->comm_send.rate = rate;
    simcall->comm_send.src_buff = src_buff;
    simcall->comm_send.src_buff_size = src_buff_size;
    simcall->comm_send.match_fun = match_fun;
    simcall->comm_send.data = data;
    simcall->comm_send.timeout = timeout;

    SIMIX_simcall_push(simcall->issuer);
  }
}

smx_action_t simcall_comm_isend(smx_rdv_t rdv, double task_size, double rate,
                              void *src_buff, size_t src_buff_size,
                              int (*match_fun)(void *, void *),
                              void (*clean_fun)(void *),
                              void *data,
                              int detached)
{
  /* checking for infinite values */
  xbt_assert(isfinite(task_size), "task_size is not finite!");
  xbt_assert(isfinite(rate), "rate is not finite!");
  
  xbt_assert(rdv, "No rendez-vous point defined for isend");

  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_ISEND;
  simcall->comm_isend.rdv = rdv;
  simcall->comm_isend.task_size = task_size;
  simcall->comm_isend.rate = rate;
  simcall->comm_isend.src_buff = src_buff;
  simcall->comm_isend.src_buff_size = src_buff_size;
  simcall->comm_isend.match_fun = match_fun;
  simcall->comm_isend.clean_fun = clean_fun;
  simcall->comm_isend.data = data;
  simcall->comm_isend.detached = detached;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_isend.result;
}

void simcall_comm_recv(smx_rdv_t rdv, void *dst_buff, size_t * dst_buff_size,
                         int (*match_fun)(void *, void *), void *data, double timeout)
{
  xbt_assert(isfinite(timeout), "timeout is not finite!");
  xbt_assert(rdv, "No rendez-vous point defined for recv");

  if (MC_IS_ENABLED) {
    /* the model-checker wants two separate simcalls */
    smx_action_t comm = simcall_comm_irecv(rdv, dst_buff, dst_buff_size,
        match_fun, data);
    simcall_comm_wait(comm, timeout);
  }
  else {
    smx_simcall_t simcall = SIMIX_simcall_mine();

    simcall->call = SIMCALL_COMM_RECV;
    simcall->comm_recv.rdv = rdv;
    simcall->comm_recv.dst_buff = dst_buff;
    simcall->comm_recv.dst_buff_size = dst_buff_size;
    simcall->comm_recv.match_fun = match_fun;
    simcall->comm_recv.data = data;
    simcall->comm_recv.timeout = timeout;

    SIMIX_simcall_push(simcall->issuer);
  }
}

smx_action_t simcall_comm_irecv(smx_rdv_t rdv, void *dst_buff, size_t * dst_buff_size,
                                  int (*match_fun)(void *, void *), void *data)
{
  xbt_assert(rdv, "No rendez-vous point defined for irecv");

  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_IRECV;
  simcall->comm_irecv.rdv = rdv;
  simcall->comm_irecv.dst_buff = dst_buff;
  simcall->comm_irecv.dst_buff_size = dst_buff_size;
  simcall->comm_irecv.match_fun = match_fun;
  simcall->comm_irecv.data = data;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_irecv.result;
}

void simcall_comm_destroy(smx_action_t comm)
{
  xbt_assert(comm, "Invalid parameter");

  /* FIXME remove this simcall type: comms are auto-destroyed now */

  /*
  smx_simcall_t simcall = simcall_mine();

  simcall->call = SIMCALL_COMM_DESTROY;
  simcall->comm_destroy.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
  */
}

void simcall_comm_cancel(smx_action_t comm)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_CANCEL;
  simcall->comm_cancel.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
}

unsigned int simcall_comm_waitany(xbt_dynar_t comms)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_WAITANY;
  simcall->comm_waitany.comms = comms;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_waitany.result;
}

int simcall_comm_testany(xbt_dynar_t comms)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();
  if (xbt_dynar_is_empty(comms))
    return -1;

  simcall->call = SIMCALL_COMM_TESTANY;
  simcall->comm_testany.comms = comms;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_testany.result;
}

void simcall_comm_wait(smx_action_t comm, double timeout)
{
  xbt_assert(isfinite(timeout), "timeout is not finite!");
  
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_WAIT;
  simcall->comm_wait.comm = comm;
  simcall->comm_wait.timeout = timeout;

  SIMIX_simcall_push(simcall->issuer);
}

#ifdef HAVE_TRACING
/**
 * \brief Set the category of an action.
 *
 * This functions changes the category only. It calls a surf function.
 * \param execution The execution action
 * \param category The tracing category
 */
void simcall_set_category(smx_action_t action, const char *category)
{
  if (category == NULL) {
    return;
  }

  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_SET_CATEGORY;
  simcall->set_category.action = action;
  simcall->set_category.category = category;

  SIMIX_simcall_push(simcall->issuer);
}
#endif

int simcall_comm_test(smx_action_t comm)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_TEST;
  simcall->comm_test.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_test.result;
}

double simcall_comm_get_remains(smx_action_t comm)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_GET_REMAINS;
  simcall->comm_get_remains.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_get_remains.result;
}

e_smx_state_t simcall_comm_get_state(smx_action_t comm)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_GET_STATE;
  simcall->comm_get_state.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_get_state.result;
}

void *simcall_comm_get_src_data(smx_action_t comm)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_GET_SRC_DATA;
  simcall->comm_get_src_data.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_get_src_data.result;
}

void *simcall_comm_get_dst_data(smx_action_t comm)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_GET_DST_DATA;
  simcall->comm_get_dst_data.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_get_dst_data.result;
}

smx_process_t simcall_comm_get_src_proc(smx_action_t comm)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_GET_SRC_PROC;
  simcall->comm_get_src_proc.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_get_src_proc.result;
}

smx_process_t simcall_comm_get_dst_proc(smx_action_t comm)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_GET_DST_PROC;
  simcall->comm_get_dst_proc.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_get_dst_proc.result;
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
int simcall_comm_is_latency_bounded(smx_action_t comm)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COMM_IS_LATENCY_BOUNDED;
  simcall->comm_is_latency_bounded.comm = comm;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->comm_is_latency_bounded.result;
}
#endif

smx_mutex_t simcall_mutex_init(void)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_MUTEX_INIT;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->mutex_init.result;
}

void simcall_mutex_destroy(smx_mutex_t mutex)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_MUTEX_DESTROY;
  simcall->mutex_destroy.mutex = mutex;

  SIMIX_simcall_push(simcall->issuer);
}

void simcall_mutex_lock(smx_mutex_t mutex)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_MUTEX_LOCK;
  simcall->mutex_lock.mutex = mutex;

  SIMIX_simcall_push(simcall->issuer);
}

int simcall_mutex_trylock(smx_mutex_t mutex)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_MUTEX_TRYLOCK;
  simcall->mutex_trylock.mutex = mutex;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->mutex_trylock.result;
}

void simcall_mutex_unlock(smx_mutex_t mutex)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_MUTEX_UNLOCK;
  simcall->mutex_unlock.mutex = mutex;

  SIMIX_simcall_push(simcall->issuer);
}


smx_cond_t simcall_cond_init(void)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COND_INIT;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->cond_init.result;
}

void simcall_cond_destroy(smx_cond_t cond)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COND_DESTROY;
  simcall->cond_destroy.cond = cond;

  SIMIX_simcall_push(simcall->issuer);
}

void simcall_cond_signal(smx_cond_t cond)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COND_SIGNAL;
  simcall->cond_signal.cond = cond;

  SIMIX_simcall_push(simcall->issuer);
}

void simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COND_WAIT;
  simcall->cond_wait.cond = cond;
  simcall->cond_wait.mutex = mutex;

  SIMIX_simcall_push(simcall->issuer);
}

void simcall_cond_wait_timeout(smx_cond_t cond,
                                 smx_mutex_t mutex,
                                 double timeout)
{
  xbt_ex_t e;

  xbt_assert(isfinite(timeout), "timeout is not finite!");
  
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COND_WAIT_TIMEOUT;
  simcall->cond_wait_timeout.cond = cond;
  simcall->cond_wait_timeout.mutex = mutex;
  simcall->cond_wait_timeout.timeout = timeout;

  TRY {
    SIMIX_simcall_push(simcall->issuer);
  }
  CATCH(e) {
    switch (e.category) {
      case timeout_error:
        simcall->issuer->waiting_action = NULL; // FIXME: should clean ?
        break;
      default:
        break;
    }
    RETHROW;
    xbt_ex_free(e);
  }
}

void simcall_cond_broadcast(smx_cond_t cond)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_COND_BROADCAST;
  simcall->cond_broadcast.cond = cond;

  SIMIX_simcall_push(simcall->issuer);
}


smx_sem_t simcall_sem_init(int capacity)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_SEM_INIT;
  simcall->sem_init.capacity = capacity;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->sem_init.result;
}

void simcall_sem_destroy(smx_sem_t sem)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_SEM_DESTROY;
  simcall->sem_destroy.sem = sem;

  SIMIX_simcall_push(simcall->issuer);
}

void simcall_sem_release(smx_sem_t sem)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_SEM_RELEASE;
  simcall->sem_release.sem = sem;

  SIMIX_simcall_push(simcall->issuer);
}

int simcall_sem_would_block(smx_sem_t sem)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_SEM_WOULD_BLOCK;
  simcall->sem_would_block.sem = sem;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->sem_would_block.result;
}

void simcall_sem_acquire(smx_sem_t sem)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_SEM_ACQUIRE;
  simcall->sem_acquire.sem = sem;

  SIMIX_simcall_push(simcall->issuer);
}

void simcall_sem_acquire_timeout(smx_sem_t sem, double timeout)
{
  xbt_assert(isfinite(timeout), "timeout is not finite!");
  
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_SEM_ACQUIRE_TIMEOUT;
  simcall->sem_acquire_timeout.sem = sem;
  simcall->sem_acquire_timeout.timeout = timeout;

  SIMIX_simcall_push(simcall->issuer);
}

int simcall_sem_get_capacity(smx_sem_t sem)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_SEM_GET_CAPACITY;
  simcall->sem_get_capacity.sem = sem;

  SIMIX_simcall_push(simcall->issuer);
  return simcall->sem_get_capacity.result;
}

size_t simcall_file_read(const char* storage, void* ptr, size_t size, size_t nmemb, smx_file_t stream)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_FILE_READ;
  simcall->file_read.storage = storage;
  simcall->file_read.ptr = ptr;
  simcall->file_read.size = size;
  simcall->file_read.nmemb = nmemb;
  simcall->file_read.stream = stream;
  SIMIX_simcall_push(simcall->issuer);

  return simcall->file_read.result;
}

size_t simcall_file_write(const char* storage, const void* ptr, size_t size, size_t nmemb, smx_file_t stream)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_FILE_WRITE;
  simcall->file_write.storage = storage;
  simcall->file_write.ptr = ptr;
  simcall->file_write.size = size;
  simcall->file_write.nmemb = nmemb;
  simcall->file_write.stream = stream;
  SIMIX_simcall_push(simcall->issuer);

  return simcall->file_write.result;
}

smx_file_t simcall_file_open(const char* storage, const char* path, const char* mode)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_FILE_OPEN;
  simcall->file_open.storage = storage;
  simcall->file_open.path = path;
  simcall->file_open.mode = mode;
  SIMIX_simcall_push(simcall->issuer);

  return simcall->file_open.result;
}

int simcall_file_close(const char* storage, smx_file_t fp)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();

  simcall->call = SIMCALL_FILE_CLOSE;
  simcall->file_close.storage = storage;
  simcall->file_close.fp = fp;
  SIMIX_simcall_push(simcall->issuer);

  return simcall->file_close.result;
}

int simcall_file_stat(const char* storage, smx_file_t fd, s_file_stat_t *buf)
{
  smx_simcall_t simcall = SIMIX_simcall_mine();
  simcall->call = SIMCALL_FILE_STAT;
  simcall->file_stat.storage = storage;
  simcall->file_stat.fd = fd;

  SIMIX_simcall_push(simcall->issuer);

  *buf = simcall->file_stat.buf;

  return simcall->file_stat.result;
}

/* ************************************************************************** */

/** @brief returns a printable string representing a simcall */
const char *SIMIX_simcall_name(e_smx_simcall_t kind) {
  return simcall_names[kind];
}
