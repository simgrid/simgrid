/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "src/simix/smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** @addtogroup m_task_management
 *
 *  Since most scheduling algorithms rely on a concept of task  that can be either <em>computed</em> locally or
 *  <em>transferred</em> on another processor, it seems to be the right level of abstraction for our purposes.
 *  A <em>task</em> may then be defined by a <em>computing amount</em>, a <em>message size</em> and
 *  some <em>private data</em>.
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_task, msg, "Logging specific to MSG (task)");

/********************************* Task **************************************/
/** \ingroup m_task_management
 * \brief Creates a new #msg_task_t.
 *
 * A constructor for #msg_task_t taking four arguments and returning the corresponding object.
 * \param name a name for the object. It is for user-level information and can be NULL.
 * \param flop_amount a value of the processing amount (in flop) needed to process this new task.
 * If 0, then it cannot be executed with MSG_task_execute(). This value has to be >=0.
 * \param message_size a value of the amount of data (in bytes) needed to transfer this new task. If 0, then it cannot
 * be transfered with MSG_task_send() and MSG_task_recv(). This value has to be >=0.
 * \param data a pointer to any data may want to attach to the new object.  It is for user-level information and can
 * be NULL. It can be retrieved with the function \ref MSG_task_get_data.
 * \see msg_task_t
 * \return The new corresponding object.
 */
msg_task_t MSG_task_create(const char *name, double flop_amount, double message_size, void *data)
{
  msg_task_t task = xbt_new(s_msg_task_t, 1);
  simdata_task_t simdata = xbt_new(s_simdata_task_t, 1);
  task->simdata = simdata;

  /* Task structure */
  task->name = xbt_strdup(name);
  task->data = data;

  /* Simulator Data */
  simdata->compute = NULL;
  simdata->comm = NULL;
  simdata->bytes_amount = message_size;
  simdata->flops_amount = flop_amount;
  simdata->sender = NULL;
  simdata->receiver = NULL;
  simdata->source = NULL;
  simdata->priority = 1.0;
  simdata->bound = 0;
  simdata->affinity_mask_db = xbt_dict_new_homogeneous(NULL);
  simdata->rate = -1.0;
  simdata->isused = 0;

  simdata->host_nb = 0;
  simdata->host_list = NULL;
  simdata->flops_parallel_amount = NULL;
  simdata->bytes_parallel_amount = NULL;
  TRACE_msg_task_create(task);

  return task;
}

/** \ingroup m_task_management
 * \brief Creates a new #msg_task_t (a parallel one....).
 *
 * A constructor for #msg_task_t taking six arguments and returning the corresponding object.
 * \param name a name for the object. It is for user-level information and can be NULL.
 * \param host_nb the number of hosts implied in the parallel task.
 * \param host_list an array of \p host_nb msg_host_t.
 * \param flops_amount an array of \p host_nb doubles.
 *        flops_amount[i] is the total number of operations that have to be performed on host_list[i].
 * \param bytes_amount an array of \p host_nb* \p host_nb doubles.
 * \param data a pointer to any data may want to attach to the new object.
 *             It is for user-level information and can be NULL.
 *             It can be retrieved with the function \ref MSG_task_get_data.
 * \see msg_task_t
 * \return The new corresponding object.
 */
msg_task_t MSG_parallel_task_create(const char *name, int host_nb, const msg_host_t * host_list,
                                    double *flops_amount, double *bytes_amount, void *data)
{
  msg_task_t task = MSG_task_create(name, 0, 0, data);
  simdata_task_t simdata = task->simdata;
  int i;

  /* Simulator Data specific to parallel tasks */
  simdata->host_nb = host_nb;
  simdata->host_list = xbt_new0(sg_host_t, host_nb);
  simdata->flops_parallel_amount = flops_amount;
  simdata->bytes_parallel_amount = bytes_amount;

  for (i = 0; i < host_nb; i++)
    simdata->host_list[i] = host_list[i];

  return task;
}

/*************** Begin GPU ***************/
/** \ingroup m_task_management
 * \brief Creates a new #msg_gpu_task_t.

 * A constructor for #msg_gpu_task_t taking four arguments and returning a pointer to the new created GPU task.

 * \param name a name for the object. It is for user-level information and can be NULL.
 * \param flops_amount a value of the processing amount (in flop)needed to process this new task. If 0, then it cannot
 * be executed with MSG_gpu_task_execute(). This value has to be >=0.
 * \param dispatch_latency time in seconds to load this task on the GPU
 * \param collect_latency time in seconds to transfer result from the GPU back to the CPU (host) when done

 * \see msg_gpu_task_t
 * \return The new corresponding object.
 */
msg_gpu_task_t MSG_gpu_task_create(const char *name, double flops_amount, double dispatch_latency,
                                   double collect_latency)
{
  msg_gpu_task_t task = xbt_new(s_msg_gpu_task_t, 1);
  simdata_gpu_task_t simdata = xbt_new(s_simdata_gpu_task_t, 1);
  task->simdata = simdata;
  /* Task structure */
  task->name = xbt_strdup(name);

  /* Simulator Data */
  simdata->flops_amount = flops_amount;
  simdata->dispatch_latency   = dispatch_latency;
  simdata->collect_latency    = collect_latency;

  /* TRACE_msg_gpu_task_create(task); FIXME*/
  return task;
}
/*************** End GPU ***************/

/** \ingroup m_task_management
 * \brief Return the user data of a #msg_task_t.
 *
 * This function checks whether \a task is a valid pointer and return the user data associated to \a task if possible.
 */
void *MSG_task_get_data(msg_task_t task)
{
  xbt_assert((task != NULL), "Invalid parameter");
  return (task->data);
}

/** \ingroup m_task_management
 * \brief Sets the user data of a #msg_task_t.
 *
 * This function allows to associate a new pointer to the user data associated of \a task.
 */
void MSG_task_set_data(msg_task_t task, void *data)
{
  xbt_assert((task != NULL), "Invalid parameter");
  task->data = data;
}

/** \ingroup m_task_management
 * \brief Sets a function to be called when a task has just been copied.
 * \param callback a callback function
 */
void MSG_task_set_copy_callback(void (*callback) (msg_task_t task, msg_process_t sender, msg_process_t receiver)) {

  msg_global->task_copy_callback = callback;

  if (callback) {
    SIMIX_comm_set_copy_data_callback(MSG_comm_copy_data_from_SIMIX);
  } else {
    SIMIX_comm_set_copy_data_callback(SIMIX_comm_copy_pointer_callback);
  }
}

/** \ingroup m_task_management
 * \brief Return the sender of a #msg_task_t.
 *
 * This functions returns the #msg_process_t which sent this task
 */
msg_process_t MSG_task_get_sender(msg_task_t task)
{
  xbt_assert(task, "Invalid parameters");
  return ((simdata_task_t) task->simdata)->sender;
}

/** \ingroup m_task_management
 * \brief Return the source of a #msg_task_t.
 *
 * This functions returns the #msg_host_t from which this task was sent
 */
msg_host_t MSG_task_get_source(msg_task_t task)
{
  xbt_assert(task, "Invalid parameters");
  return ((simdata_task_t) task->simdata)->source;
}

/** \ingroup m_task_management
 * \brief Return the name of a #msg_task_t.
 *
 * This functions returns the name of a #msg_task_t as specified on creation
 */
const char *MSG_task_get_name(msg_task_t task)
{
  xbt_assert(task, "Invalid parameters");
  return task->name;
}

/** \ingroup m_task_management
 * \brief Sets the name of a #msg_task_t.
 *
 * This functions allows to associate a name to a task
 */
void MSG_task_set_name(msg_task_t task, const char *name)
{
  xbt_assert(task, "Invalid parameters");
  task->name = xbt_strdup(name);
}

/** \ingroup m_task_management
 * \brief Destroy a #msg_task_t.
 *
 * Destructor for #msg_task_t. Note that you should free user data, if any, \b before calling this function.
 *
 * Only the process that owns the task can destroy it.
 * The owner changes after a successful send.
 * If a task is successfully sent, the receiver becomes the owner and is supposed to destroy it. The sender should not
 * use it anymore.
 * If the task failed to be sent, the sender remains the owner of the task.
 */
msg_error_t MSG_task_destroy(msg_task_t task)
{
  smx_synchro_t action = NULL;
  xbt_assert((task != NULL), "Invalid parameter");

  if (task->simdata->isused) {
    /* the task is being sent or executed: cancel it first */
    MSG_task_cancel(task);
  }
  TRACE_msg_task_destroy(task);

  xbt_free(task->name);

  action = task->simdata->compute;
  if (action)
    simcall_execution_destroy(action);

  /* parallel tasks only */
  xbt_free(task->simdata->host_list);

  xbt_dict_free(&task->simdata->affinity_mask_db);

  /* free main structures */
  xbt_free(task->simdata);
  xbt_free(task);

  return MSG_OK;
}

/** \ingroup m_task_usage
 * \brief Cancel a #msg_task_t.
 * \param task the task to cancel. If it was executed or transfered, it stops the process that were working on it.
 */
msg_error_t MSG_task_cancel(msg_task_t task)
{
  xbt_assert((task != NULL), "Cannot cancel a NULL task");

  if (task->simdata->compute) {
    simcall_execution_cancel(task->simdata->compute);
  }
  else if (task->simdata->comm) {
    simdata_task_t simdata = task->simdata;
    simcall_comm_cancel(simdata->comm);
    if (msg_global->debug_multiple_use && simdata->isused!=0)
      xbt_ex_free(*(xbt_ex_t*)simdata->isused);
    simdata->isused = 0;
  }
  return MSG_OK;
}

/** \ingroup m_task_management
 * \brief Returns the remaining amount of flops needed to execute a task #msg_task_t.
 *
 * Once a task has been processed, this amount is set to 0. If you want, you can reset this value with
 * #MSG_task_set_flops_amount before restarting the task.
 */
double MSG_task_get_flops_amount(msg_task_t task) {
  if (task->simdata->compute) {
    return simcall_execution_get_remains(task->simdata->compute);
  } else {
    return task->simdata->flops_amount;
  }
}

/** \ingroup m_task_management
 * \brief set the computation amount needed to process a task #msg_task_t.
 *
 * \warning If the computation is ongoing (already started and not finished),
 * it is not modified by this call. Moreover, after its completion, the ongoing execution with set the flops_amount to
 * zero, overriding any value set during the execution.
 */
void MSG_task_set_flops_amount(msg_task_t task, double flops_amount)
{
  task->simdata->flops_amount = flops_amount;
}

/** \ingroup m_task_management
 * \brief set the amount data attached with a task #msg_task_t.
 *
 * \warning If the transfer is ongoing (already started and not finished), it is not modified by this call.
 */
void MSG_task_set_bytes_amount(msg_task_t task, double data_size)
{
  task->simdata->bytes_amount = data_size;
}

/** \ingroup m_task_management
 * \brief Returns the total amount received by a task #msg_task_t.
 *        If the communication does not exist it will return 0.
 *        So, if the communication has FINISHED or FAILED it returns zero.
 */
double MSG_task_get_remaining_communication(msg_task_t task)
{
  xbt_assert((task != NULL) && (task->simdata != NULL), "Invalid parameter");
  XBT_DEBUG("calling simcall_communication_get_remains(%p)", task->simdata->comm);
  return simcall_comm_get_remains(task->simdata->comm);
}

/** \ingroup m_task_management
 * \brief Returns the size of the data attached to a task #msg_task_t.
 */
double MSG_task_get_bytes_amount(msg_task_t task)
{
  xbt_assert((task != NULL) && (task->simdata != NULL), "Invalid parameter");
  return task->simdata->bytes_amount;
}

/** \ingroup m_task_management
 * \brief Changes the priority of a computation task. This priority doesn't affect the transfer rate. A priority of 2
 *        will make a task receive two times more cpu power than the other ones.
 */
void MSG_task_set_priority(msg_task_t task, double priority)
{
  xbt_assert((task != NULL) && (task->simdata != NULL), "Invalid parameter");
  task->simdata->priority = 1 / priority;
  if (task->simdata->compute)
    simcall_execution_set_priority(task->simdata->compute,
        task->simdata->priority);
}

/** \ingroup m_task_management
 * \brief Changes the maximum CPU utilization of a computation task.
 *        Unit is flops/s.
 *
 * For VMs, there is a pitfall. Please see MSG_vm_set_bound().
 */
void MSG_task_set_bound(msg_task_t task, double bound)
{
  xbt_assert(task, "Invalid parameter");
  xbt_assert(task->simdata, "Invalid parameter");

  if (bound == 0)
    XBT_INFO("bound == 0 means no capping (i.e., unlimited).");

  task->simdata->bound = bound;
  if (task->simdata->compute)
    simcall_execution_set_bound(task->simdata->compute, task->simdata->bound);
}

/** \ingroup m_task_management
 * \brief Changes the CPU affinity of a computation task.
 *
 * When pinning the given task to the first CPU core of the given host, use 0x01 for the mask value. Each bit of the
 * mask value corresponds to each CPU core. See taskset(1) on Linux.
 *
 * \param task a target task
 * \param host the host having a multi-core CPU
 * \param mask the bit mask of a new CPU affinity setting for the task
 *
 * Usage:
 * 0. Define a host with multiple cores.
 *    \<host id="PM0" power="1E8" core="2"/\>
 *
 * 1. Pin a given task to the first CPU core of a host.
 *   MSG_task_set_affinity(task, pm0, 0x01);
 *
 * 2. Pin a given task to the third CPU core of a host. Turn on the third bit of the mask.
 *   MSG_task_set_affinity(task, pm0, 0x04); // 0x04 == 100B
 *
 * 3. Pin a given VM to the first CPU core of a host.
 *   MSG_vm_set_affinity(vm, pm0, 0x01);
 *
 * See examples/msg/cloud/multicore.c for more information.
 *
 * Note:
 * 1. The current code does not allow an affinity of a task to multiple cores.
 *    The mask value 0x03 (i.e., a given task will be executed on the first core or the second core) is not allowed.
 *    The mask value 0x01 or 0x02 works. See cpu_cas01.c for details.
 *
 * 2. It is recommended to first compare simulation results in both the Lazy and Full calculation modes
 *    (using --cfg=cpu/optim:Full or not). Fix cpu_cas01.c if you find wrong results in the Lazy mode.
 */
void MSG_task_set_affinity(msg_task_t task, msg_host_t host, unsigned long mask)
{
  xbt_assert(task, "Invalid parameter");
  xbt_assert(task->simdata, "Invalid parameter");

  if (mask == 0) {
    /* 0 means clear */
      /* We need remove_ext() not throwing exception. */
      void *ret = xbt_dict_get_or_null_ext(task->simdata->affinity_mask_db, (char *) host, sizeof(msg_host_t));
      if (ret != NULL)
        xbt_dict_remove_ext(task->simdata->affinity_mask_db, (char *) host, sizeof(host));
  } else
    xbt_dict_set_ext(task->simdata->affinity_mask_db, (char *) host, sizeof(host), (void *)(uintptr_t) mask, NULL);

  /* We set affinity data of this task. If the task is being executed, we actually change the affinity setting of the
   * task. Otherwise, this change will be applied when the task is executed. */
  if (!task->simdata->compute) {
    /* task is not yet executed */
    XBT_INFO("set affinity(0x%04lx@%s) for %s (not active now)", mask, MSG_host_get_name(host),
             MSG_task_get_name(task));
    return;
  }

  {
    smx_synchro_t compute = task->simdata->compute;
    msg_host_t host_now = compute->execution.host;  // simix_private.h is necessary
    if (host_now != host) {
      /* task is not yet executed on this host */
      XBT_INFO("set affinity(0x%04lx@%s) for %s (not active now)", mask, MSG_host_get_name(host),
               MSG_task_get_name(task));
      return;
    }

    /* task is being executed on this host. so change the affinity now */
    {
      /* check it works. remove me if it works. */
      xbt_assert((unsigned long)(uintptr_t) xbt_dict_get_or_null_ext(task->simdata->affinity_mask_db,
                 (char *) host, sizeof(msg_host_t)) == mask);
    }

    XBT_INFO("set affinity(0x%04lx@%s) for %s", mask, MSG_host_get_name(host), MSG_task_get_name(task));
    simcall_execution_set_affinity(task->simdata->compute, host, mask);
  }
}
