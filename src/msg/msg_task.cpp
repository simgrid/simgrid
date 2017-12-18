/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.hpp"
#include "src/simix/smx_private.hpp"
#include <algorithm>

extern "C" {

/** @addtogroup m_task_management
 *
 *  Since most scheduling algorithms rely on a concept of task  that can be either <em>computed</em> locally or
 *  <em>transferred</em> on another processor, it seems to be the right level of abstraction for our purposes.
 *  A <em>task</em> may then be defined by a <em>computing amount</em>, a <em>message size</em> and
 *  some <em>private data</em>.
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_task, msg, "Logging specific to MSG (task)");

void s_simdata_task_t::reportMultipleUse() const
{
  if (msg_global->debug_multiple_use){
    XBT_ERROR("This task is already used in there:");
    // TODO, backtrace
    XBT_ERROR("<missing backtrace>");
    XBT_ERROR("And you try to reuse it from here:");
    xbt_backtrace_display_current();
  } else {
    xbt_die("This task is still being used somewhere else. You cannot send it now. Go fix your code!"
             "(use --cfg=msg/debug-multiple-use:on to get the backtrace of the other process)");
  }
}

/********************************* Task **************************************/
/** \ingroup m_task_management
 * \brief Creates a new #msg_task_t.
 *
 * A constructor for #msg_task_t taking four arguments and returning the corresponding object.
 * \param name a name for the object. It is for user-level information and can be nullptr.
 * \param flop_amount a value of the processing amount (in flop) needed to process this new task.
 * If 0, then it cannot be executed with MSG_task_execute(). This value has to be >=0.
 * \param message_size a value of the amount of data (in bytes) needed to transfer this new task. If 0, then it cannot
 * be transfered with MSG_task_send() and MSG_task_recv(). This value has to be >=0.
 * \param data a pointer to any data may want to attach to the new object.  It is for user-level information and can
 * be nullptr. It can be retrieved with the function \ref MSG_task_get_data.
 * \see msg_task_t
 * \return The new corresponding object.
 */
msg_task_t MSG_task_create(const char *name, double flop_amount, double message_size, void *data)
{
  msg_task_t task        = new s_msg_task_t;
  simdata_task_t simdata = new s_simdata_task_t();
  task->simdata = simdata;

  /* Task structure */
  task->name = xbt_strdup(name);
  task->data = data;

  /* Simulator Data */
  simdata->bytes_amount = message_size;
  simdata->flops_amount = flop_amount;

  TRACE_msg_task_create(task);

  return task;
}

/** \ingroup m_task_management
 * \brief Creates a new #msg_task_t (a parallel one....).
 *
 * A constructor for #msg_task_t taking six arguments and returning the corresponding object.
 * \param name a name for the object. It is for user-level information and can be nullptr.
 * \param host_nb the number of hosts implied in the parallel task.
 * \param host_list an array of \p host_nb msg_host_t.
 * \param flops_amount an array of \p host_nb doubles.
 *        flops_amount[i] is the total number of operations that have to be performed on host_list[i].
 * \param bytes_amount an array of \p host_nb* \p host_nb doubles.
 * \param data a pointer to any data may want to attach to the new object.
 *             It is for user-level information and can be nullptr.
 *             It can be retrieved with the function \ref MSG_task_get_data.
 * \see msg_task_t
 * \return The new corresponding object.
 */
msg_task_t MSG_parallel_task_create(const char *name, int host_nb, const msg_host_t * host_list,
                                    double *flops_amount, double *bytes_amount, void *data)
{
  msg_task_t task = MSG_task_create(name, 0, 0, data);
  simdata_task_t simdata = task->simdata;

  /* Simulator Data specific to parallel tasks */
  simdata->host_nb = host_nb;
  simdata->host_list             = new sg_host_t[host_nb];
  std::copy_n(host_list, host_nb, simdata->host_list);
  if (flops_amount != nullptr) {
    simdata->flops_parallel_amount = new double[host_nb];
    std::copy_n(flops_amount, host_nb, simdata->flops_parallel_amount);
  }
  if (bytes_amount != nullptr) {
    simdata->bytes_parallel_amount = new double[host_nb * host_nb];
    std::copy_n(bytes_amount, host_nb * host_nb, simdata->bytes_parallel_amount);
  }

  return task;
}

/** \ingroup m_task_management
 * \brief Return the user data of a #msg_task_t.
 *
 * This function checks whether \a task is a valid pointer and return the user data associated to \a task if possible.
 */
void *MSG_task_get_data(msg_task_t task)
{
  return (task->data);
}

/** \ingroup m_task_management
 * \brief Sets the user data of a #msg_task_t.
 *
 * This function allows to associate a new pointer to the user data associated of \a task.
 */
void MSG_task_set_data(msg_task_t task, void *data)
{
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
  return task->simdata->sender;
}

/** \ingroup m_task_management
 * \brief Return the source of a #msg_task_t.
 *
 * This functions returns the #msg_host_t from which this task was sent
 */
msg_host_t MSG_task_get_source(msg_task_t task)
{
  return task->simdata->source;
}

/** \ingroup m_task_management
 * \brief Return the name of a #msg_task_t.
 *
 * This functions returns the name of a #msg_task_t as specified on creation
 */
const char *MSG_task_get_name(msg_task_t task)
{
  return task->name;
}

/** \ingroup m_task_management
 * \brief Sets the name of a #msg_task_t.
 *
 * This functions allows to associate a name to a task
 */
void MSG_task_set_name(msg_task_t task, const char *name)
{
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
  if (task->simdata->isused) {
    /* the task is being sent or executed: cancel it first */
    MSG_task_cancel(task);
  }
  TRACE_msg_task_destroy(task);

  xbt_free(task->name);

  /* free main structures */
  delete task->simdata;
  delete task;

  return MSG_OK;
}

/** \ingroup m_task_usage
 * \brief Cancel a #msg_task_t.
 * \param task the task to cancel. If it was executed or transfered, it stops the process that were working on it.
 */
msg_error_t MSG_task_cancel(msg_task_t task)
{
  xbt_assert((task != nullptr), "Cannot cancel a nullptr task");

  simdata_task_t simdata = task->simdata;
  if (simdata->compute) {
    simcall_execution_cancel(simdata->compute);
  } else if (simdata->comm) {
    simcall_comm_cancel(simdata->comm);
  }
  simdata->setNotUsed();
  return MSG_OK;
}

/** \ingroup m_task_management
 * \brief Returns a value in ]0,1[ that represent the task remaining work
 *    to do: starts at 1 and goes to 0. Returns 0 if not started or finished.
 *
 * It works for either parallel or sequential tasks.
 * TODO: Improve this function by returning 1 if the task has not started
 */
double MSG_task_get_remaining_work_ratio(msg_task_t task) {

  xbt_assert((task != nullptr), "Cannot get information from a nullptr task");
  if (task->simdata->compute) {
    // Task in progress
    return task->simdata->compute->remainingRatio();
  } else {
    // Task not started or finished
    return 0;
  }
}

/** \ingroup m_task_management
 * \brief Returns the amount of flops that remain to be computed
 *
 * The returned value is initially the cost that you defined for the task, then it decreases until it reaches 0
 *
 * It works for sequential tasks, but the remaining amount of work is not a scalar value for parallel tasks.
 * So you will get an exception if you call this function on parallel tasks. Just don't do it.
 */
double MSG_task_get_flops_amount(msg_task_t task) {
  if (task->simdata->compute != nullptr) {
    return task->simdata->compute->remains();
  } else {
    // Not started or already done.
    // - Before starting, flops_amount is initially the task cost
    // - After execution, flops_amount is set to 0 (until someone uses MSG_task_set_flops_amount, if any)
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
  XBT_DEBUG("calling simcall_communication_get_remains(%p)", task->simdata->comm.get());
  return task->simdata->comm->remains();
}

/** \ingroup m_task_management
 * \brief Returns the size of the data attached to a task #msg_task_t.
 */
double MSG_task_get_bytes_amount(msg_task_t task)
{
  xbt_assert((task != nullptr) && (task->simdata != nullptr), "Invalid parameter");
  return task->simdata->bytes_amount;
}

/** \ingroup m_task_management
 * \brief Changes the priority of a computation task. This priority doesn't affect the transfer rate. A priority of 2
 *        will make a task receive two times more cpu power than the other ones.
 */
void MSG_task_set_priority(msg_task_t task, double priority)
{
  task->simdata->priority = 1 / priority;
  if (task->simdata->compute)
    simcall_execution_set_priority(task->simdata->compute, task->simdata->priority);
}

/** \ingroup m_task_management
 * \brief Changes the maximum CPU utilization of a computation task.
 *        Unit is flops/s.
 *
 * For VMs, there is a pitfall. Please see MSG_vm_set_bound().
 */
void MSG_task_set_bound(msg_task_t task, double bound)
{
  if (bound < 1e-12) /* close enough to 0 without any floating precision surprise */
    XBT_INFO("bound == 0 means no capping (i.e., unlimited).");

  task->simdata->bound = bound;
  if (task->simdata->compute)
    simcall_execution_set_bound(task->simdata->compute, task->simdata->bound);
}
}
