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

#include "private.h"
#include "mc/mc.h"


XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix);

static const char* request_names[] = {
#undef SIMIX_REQ_ENUM_ELEMENT
#define SIMIX_REQ_ENUM_ELEMENT(x) #x /* generate strings from the enumeration values */
SIMIX_REQ_LIST
#undef SIMIX_REQ_ENUM_ELEMENT
};

/**
 * \brief Returns a host given its name.
 *
 * \param name The name of the host to get
 * \return The corresponding host
 */
smx_host_t SIMIX_req_host_get_by_name(const char *name)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_GET_BY_NAME;
  req->host_get_by_name.name = name;
  SIMIX_request_push(req->issuer);
  return req->host_get_by_name.result;
}

/**
 * \brief Returns the name of a host.
 *
 * \param host A SIMIX host
 * \return The name of this host
 */
const char* SIMIX_req_host_get_name(smx_host_t host)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_GET_NAME;
  req->host_get_name.host = host;
  SIMIX_request_push(req->issuer);
  return req->host_get_name.result;
}

/**
 * \brief Returns a dict of the properties assigned to a host.
 *
 * \param host A host
 * \return The properties of this host
 */
xbt_dict_t SIMIX_req_host_get_properties(smx_host_t host)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_GET_PROPERTIES;
  req->host_get_properties.host = host;
  SIMIX_request_push(req->issuer);
  return req->host_get_properties.result;
}

/**
 * \brief Returns the speed of the processor.
 *
 * The speed returned does not take into account the current load on the machine.
 * \param host A SIMIX host
 * \return The speed of this host (in Mflop/s)
 */
double SIMIX_req_host_get_speed(smx_host_t host)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_GET_SPEED;
  req->host_get_speed.host = host;
  SIMIX_request_push(req->issuer);
  return req->host_get_speed.result;
}

/**
 * \brief Returns the available speed of the processor.
 *
 * \return Speed currently available (in Mflop/s)
 */
double SIMIX_req_host_get_available_speed(smx_host_t host)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_GET_AVAILABLE_SPEED;
  req->host_get_available_speed.host = host;
  SIMIX_request_push(req->issuer);
  return req->host_get_available_speed.result;
}

/**
 * \brief Returns the state of a host.
 *
 * Two states are possible: 1 if the host is active or 0 if it has crashed.
 * \param host A SIMIX host
 * \return 1 if the host is available, 0 otherwise
 */
int SIMIX_req_host_get_state(smx_host_t host)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_GET_STATE;
  req->host_get_state.host = host;
  SIMIX_request_push(req->issuer);
  return req->host_get_state.result;
}

/**
 * \brief Returns the user data associated to a host.
 *
 * \param host SIMIX host
 * \return the user data of this host
 */
void* SIMIX_req_host_get_data(smx_host_t host)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_GET_DATA;
  req->host_get_data.host = host;
  SIMIX_request_push(req->issuer);
  return req->host_get_data.result;
}

/**
 * \brief Sets the user data associated to a host.
 *
 * The host must not have previous user data associated to it.
 * \param A host SIMIX host
 * \param data The user data to set
 */
void SIMIX_req_host_set_data(smx_host_t host, void *data)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_SET_DATA;
  req->host_set_data.host = host;
  req->host_set_data.data = data;
  SIMIX_request_push(req->issuer);
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
smx_action_t SIMIX_req_host_execute(const char *name, smx_host_t host,
                                    double computation_amount,
                                    double priority)
{
  /* checking for infinite values */
  xbt_assert(isfinite(computation_amount), "computation_amount is not finite!");
  xbt_assert(isfinite(priority), "priority is not finite!");
  
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_EXECUTE;
  req->host_execute.name = name;
  req->host_execute.host = host;
  req->host_execute.computation_amount = computation_amount;
  req->host_execute.priority = priority;
  SIMIX_request_push(req->issuer);
  return req->host_execute.result;
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
smx_action_t SIMIX_req_host_parallel_execute(const char *name,
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
  
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_PARALLEL_EXECUTE;
  req->host_parallel_execute.name = name;
  req->host_parallel_execute.host_nb = host_nb;
  req->host_parallel_execute.host_list = host_list;
  req->host_parallel_execute.computation_amount = computation_amount;
  req->host_parallel_execute.communication_amount = communication_amount;
  req->host_parallel_execute.amount = amount;
  req->host_parallel_execute.rate = rate;
  SIMIX_request_push(req->issuer);
  return req->host_parallel_execute.result;
}

/**
 * \brief Destroys an execution action.
 *
 * Destroys an action, freing its memory. This function cannot be called if there are a conditional waiting for it.
 * \param action The execution action to destroy
 */
void SIMIX_req_host_execution_destroy(smx_action_t execution)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_EXECUTION_DESTROY;
  req->host_execution_destroy.execution = execution;
  SIMIX_request_push(req->issuer);
}

/**
 * \brief Cancels an execution action.
 *
 * This functions stops the execution. It calls a surf function.
 * \param action The execution action to cancel
 */
void SIMIX_req_host_execution_cancel(smx_action_t execution)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_EXECUTION_CANCEL;
  req->host_execution_cancel.execution = execution;
  SIMIX_request_push(req->issuer);
}

/**
 * \brief Returns how much of an execution action remains to be done.
 *
 * \param Action The execution action
 * \return The remaining amount
 */
double SIMIX_req_host_execution_get_remains(smx_action_t execution)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_EXECUTION_GET_REMAINS;
  req->host_execution_get_remains.execution = execution;
  SIMIX_request_push(req->issuer);
  return req->host_execution_get_remains.result;
}

/**
 * \brief Returns the state of an execution action.
 *
 * \param execution The execution action
 * \return The state
 */
e_smx_state_t SIMIX_req_host_execution_get_state(smx_action_t execution)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_EXECUTION_GET_STATE;
  req->host_execution_get_state.execution = execution;
  SIMIX_request_push(req->issuer);
  return req->host_execution_get_state.result;
}

/**
 * \brief Changes the priority of an execution action.
 *
 * This functions changes the priority only. It calls a surf function.
 * \param execution The execution action
 * \param priority The new priority
 */
void SIMIX_req_host_execution_set_priority(smx_action_t execution, double priority)
{
  /* checking for infinite values */
  xbt_assert(isfinite(priority), "priority is not finite!");
  
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_EXECUTION_SET_PRIORITY;
  req->host_execution_set_priority.execution = execution;
  req->host_execution_set_priority.priority = priority;
  SIMIX_request_push(req->issuer);
}

/**
 * \brief Waits for the completion of an execution action and destroy it.
 *
 * \param execution The execution action
 */
e_smx_state_t SIMIX_req_host_execution_wait(smx_action_t execution)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_HOST_EXECUTION_WAIT;
  req->host_execution_wait.execution = execution;
  SIMIX_request_push(req->issuer);
  return req->host_execution_wait.result;
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
 * It can be retrieved with the function \ref SIMIX_req_process_get_data.
 * \param hostname name of the host where the new agent is executed.
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code
 * \param properties the properties of the process
 */
void SIMIX_req_process_create(smx_process_t *process, const char *name,
                              xbt_main_func_t code,
                              void *data,
                              const char *hostname,
                              int argc, char **argv,
                              xbt_dict_t properties)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_CREATE;
  req->process_create.process = process;
  req->process_create.name = name;
  req->process_create.code = code;
  req->process_create.data = data;
  req->process_create.hostname = hostname;
  req->process_create.argc = argc;
  req->process_create.argv = argv;
  req->process_create.properties = properties;
  SIMIX_request_push(req->issuer);
}

/** \brief Kills a SIMIX process.
 *
 * This function simply kills a  process.
 *
 * \param process poor victim
 */
void SIMIX_req_process_kill(smx_process_t process)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_KILL;
  req->process_kill.process = process;
  SIMIX_request_push(req->issuer);
}

/** \brief Kills all SIMIX processes.
 */
void SIMIX_req_process_killall(void)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_KILLALL;
  SIMIX_request_push(req->issuer);
}

/** \brief Cleans up a SIMIX process.
 * \param process poor victim (must have already been killed)
 */
void SIMIX_req_process_cleanup(smx_process_t process)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_CLEANUP;
  req->process_cleanup.process = process;
  SIMIX_request_push(req->issuer);
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
void SIMIX_req_process_change_host(smx_process_t process, smx_host_t dest)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_CHANGE_HOST;
  req->process_change_host.process = process;
  req->process_change_host.dest = dest;
  SIMIX_request_push(req->issuer);
}

/**
 * \brief Suspends a process.
 *
 * This function suspends the process by suspending the action
 * it was waiting for completion.
 *
 * \param process a SIMIX process
 */
void SIMIX_req_process_suspend(smx_process_t process)
{
  xbt_assert(process, "Invalid parameters");

  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_SUSPEND;
  req->process_suspend.process = process;
  SIMIX_request_push(req->issuer);
}

/**
 * \brief Resumes a suspended process.
 *
 * This function resumes a suspended process by resuming the action
 * it was waiting for completion.
 *
 * \param process a SIMIX process
 */
void SIMIX_req_process_resume(smx_process_t process)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_RESUME;
  req->process_resume.process = process;
  SIMIX_request_push(req->issuer);
}

/**
 * \brief Returns the amount of SIMIX processes in the system
 *
 * Maestro internal process is not counted, only user code processes are
 */
int SIMIX_req_process_count(void)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_COUNT;
  SIMIX_request_push(req->issuer);
  return req->process_count.result;
}

/**
 * \brief Return the user data of a #smx_process_t.
 * \param process a SIMIX process
 * \return the user data of this process
 */
void* SIMIX_req_process_get_data(smx_process_t process)
{
  if (process == SIMIX_process_self()) {
    /* avoid a request if this function is called by the process itself */
    return SIMIX_process_get_data(process);
  }

  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_GET_DATA;
  req->process_get_data.process = process;
  SIMIX_request_push(req->issuer);
  return req->process_get_data.result;
}

/**
 * \brief Set the user data of a #m_process_t.
 *
 * This functions checks whether \a process is a valid pointer or not and set the user data associated to \a process if it is possible.
 * \param process SIMIX process
 * \param data User data
 */
void SIMIX_req_process_set_data(smx_process_t process, void *data)
{
  if (process == SIMIX_process_self()) {
    /* avoid a request if this function is called by the process itself */
    SIMIX_process_self_set_data(process, data);
  }
  else {

    smx_req_t req = SIMIX_req_mine();

    req->call = REQ_PROCESS_SET_DATA;
    req->process_set_data.process = process;
    req->process_set_data.data = data;
    SIMIX_request_push(req->issuer);
  }
}

/**
 * \brief Return the location on which an agent is running.
 *
 * This functions checks whether \a process is a valid pointer or not and return the m_host_t corresponding to the location on which \a process is running.
 * \param process SIMIX process
 * \return SIMIX host
 */
smx_host_t SIMIX_req_process_get_host(smx_process_t process)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_GET_HOST;
  req->process_get_host.process = process;
  SIMIX_request_push(req->issuer);
  return req->process_get_host.result;
}

/**
 * \brief Return the name of an agent.
 *
 * This functions checks whether \a process is a valid pointer or not and return its name.
 * \param process SIMIX process
 * \return The process name
 */
const char* SIMIX_req_process_get_name(smx_process_t process)
{
  if (process == SIMIX_process_self()) {
    /* avoid a request if this function is called by the process itself */
    return process->name;
  }

  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_GET_NAME;
  req->process_get_name.process = process;
  SIMIX_request_push(req->issuer);
  return req->process_get_name.result;
}

/**
 * \brief Returns true if the process is suspended .
 *
 * This checks whether a process is suspended or not by inspecting the task on which it was waiting for the completion.
 * \param process SIMIX process
 * \return 1, if the process is suspended, else 0.
 */
int SIMIX_req_process_is_suspended(smx_process_t process)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_IS_SUSPENDED;
  req->process_is_suspended.process = process;
  SIMIX_request_push(req->issuer);
  return req->process_is_suspended.result;
}

/** \ingroup m_process_management
 * \brief Return the properties
 *
 * This functions returns the properties associated with this process
 */
xbt_dict_t SIMIX_req_process_get_properties(smx_process_t process)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_GET_PROPERTIES;
  req->process_get_properties.process = process;
  SIMIX_request_push(req->issuer);
  return req->process_get_properties.result;
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
e_smx_state_t SIMIX_req_process_sleep(double duration)
{
  /* checking for infinite values */
  xbt_assert(isfinite(duration), "duration is not finite!");
  
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_PROCESS_SLEEP;
  req->process_sleep.duration = duration;
  SIMIX_request_push(req->issuer);
  return req->process_sleep.result;
}

/**
 *  \brief Creates a new rendez-vous point
 *  \param name The name of the rendez-vous point
 *  \return The created rendez-vous point
 */
smx_rdv_t SIMIX_req_rdv_create(const char *name)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_RDV_CREATE;
  req->rdv_create.name = name;

  SIMIX_request_push(req->issuer);
  return req->rdv_create.result;
}


/**
 *  \brief Destroy a rendez-vous point
 *  \param name The rendez-vous point to destroy
 */
void SIMIX_req_rdv_destroy(smx_rdv_t rdv)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_RDV_DESTROY;
  req->rdv_destroy.rdv = rdv;

  SIMIX_request_push(req->issuer);
}

smx_rdv_t SIMIX_req_rdv_get_by_name(const char *name)
{
  xbt_assert(name != NULL, "Invalid parameter for SIMIX_req_rdv_get_by_name (name is NULL)");

  /* FIXME: this is a horrible lost of performance, so we hack it out by
   * skipping the request (for now). It won't work on distributed but
   * probably we will change MSG for that. */
/*
  smx_req_t req = SIMIX_req_mine();
  req->call = REQ_RDV_GEY_BY_NAME;
  req->rdv_get_by_name.name = name;
  SIMIX_request_push(req->issuer);
  return req->rdv_get_by_name.result;*/

  return SIMIX_rdv_get_by_name(name);
}

/**
 *  \brief counts the number of communication requests of a given host pending
 *         on a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param host The host to be counted
 *  \return The number of comm request pending in the rdv
 */
int SIMIX_req_rdv_comm_count_by_host(smx_rdv_t rdv, smx_host_t host)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_RDV_COMM_COUNT_BY_HOST;
  req->rdv_comm_count_by_host.rdv = rdv;
  req->rdv_comm_count_by_host.host = host;

  SIMIX_request_push(req->issuer);
  return req->rdv_comm_count_by_host.result;
}

/**
 *  \brief returns the communication at the head of the rendez-vous
 *  \param rdv The rendez-vous point
 *  \return The communication or NULL if empty
 */
smx_action_t SIMIX_req_rdv_get_head(smx_rdv_t rdv)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_RDV_GET_HEAD;
  req->rdv_get_head.rdv = rdv;

  SIMIX_request_push(req->issuer);
  return req->rdv_get_head.result;
}

void SIMIX_req_comm_send(smx_rdv_t rdv, double task_size, double rate,
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
    /* the model-checker wants two separate requests */
    smx_action_t comm = SIMIX_req_comm_isend(rdv, task_size, rate,
        src_buff, src_buff_size, match_fun, NULL, data, 0);
    SIMIX_req_comm_wait(comm, timeout);
  }
  else {
    smx_req_t req = SIMIX_req_mine();

    req->call = REQ_COMM_SEND;
    req->comm_send.rdv = rdv;
    req->comm_send.task_size = task_size;
    req->comm_send.rate = rate;
    req->comm_send.src_buff = src_buff;
    req->comm_send.src_buff_size = src_buff_size;
    req->comm_send.match_fun = match_fun;
    req->comm_send.data = data;
    req->comm_send.timeout = timeout;

    SIMIX_request_push(req->issuer);
  }
}

smx_action_t SIMIX_req_comm_isend(smx_rdv_t rdv, double task_size, double rate,
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

  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_ISEND;
  req->comm_isend.rdv = rdv;
  req->comm_isend.task_size = task_size;
  req->comm_isend.rate = rate;
  req->comm_isend.src_buff = src_buff;
  req->comm_isend.src_buff_size = src_buff_size;
  req->comm_isend.match_fun = match_fun;
  req->comm_isend.clean_fun = clean_fun;
  req->comm_isend.data = data;
  req->comm_isend.detached = detached;

  SIMIX_request_push(req->issuer);
  return req->comm_isend.result;
}

void SIMIX_req_comm_recv(smx_rdv_t rdv, void *dst_buff, size_t * dst_buff_size,
                         int (*match_fun)(void *, void *), void *data, double timeout)
{
  xbt_assert(isfinite(timeout), "timeout is not finite!");
  xbt_assert(rdv, "No rendez-vous point defined for recv");

  if (MC_IS_ENABLED) {
    /* the model-checker wants two separate requests */
    smx_action_t comm = SIMIX_req_comm_irecv(rdv, dst_buff, dst_buff_size,
        match_fun, data);
    SIMIX_req_comm_wait(comm, timeout);
  }
  else {
    smx_req_t req = SIMIX_req_mine();

    req->call = REQ_COMM_RECV;
    req->comm_recv.rdv = rdv;
    req->comm_recv.dst_buff = dst_buff;
    req->comm_recv.dst_buff_size = dst_buff_size;
    req->comm_recv.match_fun = match_fun;
    req->comm_recv.data = data;
    req->comm_recv.timeout = timeout;

    SIMIX_request_push(req->issuer);
  }
}

smx_action_t SIMIX_req_comm_irecv(smx_rdv_t rdv, void *dst_buff, size_t * dst_buff_size,
                                  int (*match_fun)(void *, void *), void *data)
{
  xbt_assert(rdv, "No rendez-vous point defined for irecv");

  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_IRECV;
  req->comm_irecv.rdv = rdv;
  req->comm_irecv.dst_buff = dst_buff;
  req->comm_irecv.dst_buff_size = dst_buff_size;
  req->comm_irecv.match_fun = match_fun;
  req->comm_irecv.data = data;

  SIMIX_request_push(req->issuer);
  return req->comm_irecv.result;
}

void SIMIX_req_comm_destroy(smx_action_t comm)
{
  xbt_assert(comm, "Invalid parameter");

  /* FIXME remove this request type: comms are auto-destroyed now */

  /*
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_DESTROY;
  req->comm_destroy.comm = comm;

  SIMIX_request_push(req->issuer);
  */
}

void SIMIX_req_comm_cancel(smx_action_t comm)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_CANCEL;
  req->comm_cancel.comm = comm;

  SIMIX_request_push(req->issuer);
}

unsigned int SIMIX_req_comm_waitany(xbt_dynar_t comms)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_WAITANY;
  req->comm_waitany.comms = comms;

  SIMIX_request_push(req->issuer);
  return req->comm_waitany.result;
}

int SIMIX_req_comm_testany(xbt_dynar_t comms)
{
  smx_req_t req = SIMIX_req_mine();
  if (xbt_dynar_is_empty(comms))
    return -1;

  req->call = REQ_COMM_TESTANY;
  req->comm_testany.comms = comms;

  SIMIX_request_push(req->issuer);
  return req->comm_testany.result;
}

void SIMIX_req_comm_wait(smx_action_t comm, double timeout)
{
  xbt_assert(isfinite(timeout), "timeout is not finite!");
  
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_WAIT;
  req->comm_wait.comm = comm;
  req->comm_wait.timeout = timeout;

  SIMIX_request_push(req->issuer);
}

#ifdef HAVE_TRACING
/**
 * \brief Set the category of an action.
 *
 * This functions changes the category only. It calls a surf function.
 * \param execution The execution action
 * \param category The tracing category
 */
void SIMIX_req_set_category(smx_action_t action, const char *category)
{
  if (category == NULL) {
    return;
  }

  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_SET_CATEGORY;
  req->set_category.action = action;
  req->set_category.category = category;

  SIMIX_request_push(req->issuer);
}
#endif

int SIMIX_req_comm_test(smx_action_t comm)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_TEST;
  req->comm_test.comm = comm;

  SIMIX_request_push(req->issuer);
  return req->comm_test.result;
}

double SIMIX_req_comm_get_remains(smx_action_t comm)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_GET_REMAINS;
  req->comm_get_remains.comm = comm;

  SIMIX_request_push(req->issuer);
  return req->comm_get_remains.result;
}

e_smx_state_t SIMIX_req_comm_get_state(smx_action_t comm)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_GET_STATE;
  req->comm_get_state.comm = comm;

  SIMIX_request_push(req->issuer);
  return req->comm_get_state.result;
}

void *SIMIX_req_comm_get_src_data(smx_action_t comm)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_GET_SRC_DATA;
  req->comm_get_src_data.comm = comm;

  SIMIX_request_push(req->issuer);
  return req->comm_get_src_data.result;
}

void *SIMIX_req_comm_get_dst_data(smx_action_t comm)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_GET_DST_DATA;
  req->comm_get_dst_data.comm = comm;

  SIMIX_request_push(req->issuer);
  return req->comm_get_dst_data.result;
}

smx_process_t SIMIX_req_comm_get_src_proc(smx_action_t comm)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_GET_SRC_PROC;
  req->comm_get_src_proc.comm = comm;

  SIMIX_request_push(req->issuer);
  return req->comm_get_src_proc.result;
}

smx_process_t SIMIX_req_comm_get_dst_proc(smx_action_t comm)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_GET_DST_PROC;
  req->comm_get_dst_proc.comm = comm;

  SIMIX_request_push(req->issuer);
  return req->comm_get_dst_proc.result;
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
int SIMIX_req_comm_is_latency_bounded(smx_action_t comm)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COMM_IS_LATENCY_BOUNDED;
  req->comm_is_latency_bounded.comm = comm;

  SIMIX_request_push(req->issuer);
  return req->comm_is_latency_bounded.result;
}
#endif

smx_mutex_t SIMIX_req_mutex_init(void)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_MUTEX_INIT;

  SIMIX_request_push(req->issuer);
  return req->mutex_init.result;
}

void SIMIX_req_mutex_destroy(smx_mutex_t mutex)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_MUTEX_DESTROY;
  req->mutex_destroy.mutex = mutex;

  SIMIX_request_push(req->issuer);
}

void SIMIX_req_mutex_lock(smx_mutex_t mutex)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_MUTEX_LOCK;
  req->mutex_lock.mutex = mutex;

  SIMIX_request_push(req->issuer);
}

int SIMIX_req_mutex_trylock(smx_mutex_t mutex)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_MUTEX_TRYLOCK;
  req->mutex_trylock.mutex = mutex;

  SIMIX_request_push(req->issuer);
  return req->mutex_trylock.result;
}

void SIMIX_req_mutex_unlock(smx_mutex_t mutex)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_MUTEX_UNLOCK;
  req->mutex_unlock.mutex = mutex;

  SIMIX_request_push(req->issuer);
}


smx_cond_t SIMIX_req_cond_init(void)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COND_INIT;

  SIMIX_request_push(req->issuer);
  return req->cond_init.result;
}

void SIMIX_req_cond_destroy(smx_cond_t cond)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COND_DESTROY;
  req->cond_destroy.cond = cond;

  SIMIX_request_push(req->issuer);
}

void SIMIX_req_cond_signal(smx_cond_t cond)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COND_SIGNAL;
  req->cond_signal.cond = cond;

  SIMIX_request_push(req->issuer);
}

void SIMIX_req_cond_wait(smx_cond_t cond, smx_mutex_t mutex)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COND_WAIT;
  req->cond_wait.cond = cond;
  req->cond_wait.mutex = mutex;

  SIMIX_request_push(req->issuer);
}

void SIMIX_req_cond_wait_timeout(smx_cond_t cond,
                                 smx_mutex_t mutex,
                                 double timeout)
{
  xbt_assert(isfinite(timeout), "timeout is not finite!");
  
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COND_WAIT_TIMEOUT;
  req->cond_wait_timeout.cond = cond;
  req->cond_wait_timeout.mutex = mutex;
  req->cond_wait_timeout.timeout = timeout;

  SIMIX_request_push(req->issuer);
}

void SIMIX_req_cond_broadcast(smx_cond_t cond)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_COND_BROADCAST;
  req->cond_broadcast.cond = cond;

  SIMIX_request_push(req->issuer);
}


smx_sem_t SIMIX_req_sem_init(int capacity)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_SEM_INIT;
  req->sem_init.capacity = capacity;

  SIMIX_request_push(req->issuer);
  return req->sem_init.result;
}

void SIMIX_req_sem_destroy(smx_sem_t sem)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_SEM_DESTROY;
  req->sem_destroy.sem = sem;

  SIMIX_request_push(req->issuer);
}

void SIMIX_req_sem_release(smx_sem_t sem)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_SEM_RELEASE;
  req->sem_release.sem = sem;

  SIMIX_request_push(req->issuer);
}

int SIMIX_req_sem_would_block(smx_sem_t sem)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_SEM_WOULD_BLOCK;
  req->sem_would_block.sem = sem;

  SIMIX_request_push(req->issuer);
  return req->sem_would_block.result;
}

void SIMIX_req_sem_acquire(smx_sem_t sem)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_SEM_ACQUIRE;
  req->sem_acquire.sem = sem;

  SIMIX_request_push(req->issuer);
}

void SIMIX_req_sem_acquire_timeout(smx_sem_t sem, double timeout)
{
  xbt_assert(isfinite(timeout), "timeout is not finite!");
  
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_SEM_ACQUIRE_TIMEOUT;
  req->sem_acquire_timeout.sem = sem;
  req->sem_acquire_timeout.timeout = timeout;

  SIMIX_request_push(req->issuer);
}

int SIMIX_req_sem_get_capacity(smx_sem_t sem)
{
  smx_req_t req = SIMIX_req_mine();

  req->call = REQ_SEM_GET_CAPACITY;
  req->sem_get_capacity.sem = sem;

  SIMIX_request_push(req->issuer);
  return req->sem_get_capacity.result;
}
/* ************************************************************************** */

/** @brief returns a printable string representing the request kind */
const char *SIMIX_request_name(int kind) {
  return request_names[kind];
}
