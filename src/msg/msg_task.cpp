/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.hpp"
#include "src/simix/smx_private.hpp"
#include <algorithm>
#include <cmath>
#include <simgrid/modelchecker.h>
#include <simgrid/s4u/Comm.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_task, msg, "Logging specific to MSG (task)");

namespace simgrid {
namespace msg {
Task::~Task()
{
  /* parallel tasks only */
  delete[] host_list;
  delete[] flops_parallel_amount;
  delete[] bytes_parallel_amount;
}

void Task::set_used()
{
  if (this->is_used)
    this->report_multiple_use();
  this->is_used = true;
}

void Task::report_multiple_use() const
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
} // namespace msg
} // namespace simgrid

/********************************* Task **************************************/
/** @brief Creates a new task
 *
 * A constructor for msg_task_t taking four arguments.
 *
 * @param name a name for the object. It is for user-level information and can be nullptr.
 * @param flop_amount a value of the processing amount (in flop) needed to process this new task.
 * If 0, then it cannot be executed with MSG_task_execute(). This value has to be >=0.
 * @param message_size a value of the amount of data (in bytes) needed to transfer this new task. If 0, then it cannot
 * be transfered with MSG_task_send() and MSG_task_recv(). This value has to be >=0.
 * @param data a pointer to any data may want to attach to the new object.  It is for user-level information and can
 * be nullptr. It can be retrieved with the function @ref MSG_task_get_data.
 * @return The new corresponding object.
 */
msg_task_t MSG_task_create(const char *name, double flop_amount, double message_size, void *data)
{
  static std::atomic_ullong counter{0};

  msg_task_t task        = new s_msg_task_t;
  /* Simulator Data */
  task->simdata = new simgrid::msg::Task(name ? name : "", flop_amount, message_size, data);

  /* Task structure */
  task->counter  = counter++;

  if (MC_is_active())
    MC_ignore_heap(&(task->counter), sizeof(task->counter));

  return task;
}

/** @brief Creates a new parallel task
 *
 * A constructor for #msg_task_t taking six arguments.
 *
 * \rst
 * See :cpp:func:`void simgrid::s4u::this_actor::parallel_execute(int, s4u::Host**, double*, double*)` for
 * the exact semantic of the parameters.
 * \endrst
 *
 * @param name a name for the object. It is for user-level information and can be nullptr.
 * @param host_nb the number of hosts implied in the parallel task.
 * @param host_list an array of @p host_nb msg_host_t.
 * @param flops_amount an array of @p host_nb doubles.
 *        flops_amount[i] is the total number of operations that have to be performed on host_list[i].
 * @param bytes_amount an array of @p host_nb* @p host_nb doubles.
 * @param data a pointer to any data may want to attach to the new object.
 *             It is for user-level information and can be nullptr.
 *             It can be retrieved with the function @ref MSG_task_get_data().
 */
msg_task_t MSG_parallel_task_create(const char *name, int host_nb, const msg_host_t * host_list,
                                    double *flops_amount, double *bytes_amount, void *data)
{
  // Task's flops amount is set to an arbitrary value > 0.0 to be able to distinguish, in
  // MSG_task_get_remaining_work_ratio(), a finished task and a task that has not started yet.
  msg_task_t task        = MSG_task_create(name, 1.0, 0, data);
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

/** @brief Return the user data of the given task */
void* MSG_task_get_data(msg_task_t task)
{
  return (task->simdata->get_user_data());
}

/** @brief Sets the user data of a given task */
void MSG_task_set_data(msg_task_t task, void *data)
{
  task->simdata->set_user_data(data);
}

/** @brief Sets a function to be called when a task has just been copied.
 * @param callback a callback function
 */
void MSG_task_set_copy_callback(void (*callback) (msg_task_t task, msg_process_t sender, msg_process_t receiver)) {

  msg_global->task_copy_callback = callback;

  if (callback) {
    SIMIX_comm_set_copy_data_callback(MSG_comm_copy_data_from_SIMIX);
  } else {
    SIMIX_comm_set_copy_data_callback(SIMIX_comm_copy_pointer_callback);
  }
}

/** @brief Returns the sender of the given task */
msg_process_t MSG_task_get_sender(msg_task_t task)
{
  return task->simdata->sender;
}

/** @brief Returns the source (the sender's host) of the given task */
msg_host_t MSG_task_get_source(msg_task_t task)
{
  return task->simdata->sender->get_host();
}

/** @brief Returns the name of the given task. */
const char *MSG_task_get_name(msg_task_t task)
{
  return task->simdata->get_cname();
}

/** @brief Sets the name of the given task. */
void MSG_task_set_name(msg_task_t task, const char *name)
{
  task->simdata->set_name(name);
}

/** @brief Destroys the given task.
 *
 * You should free user data, if any, @b before calling this destructor.
 *
 * Only the process that owns the task can destroy it.
 * The owner changes after a successful send.
 * If a task is successfully sent, the receiver becomes the owner and is supposed to destroy it. The sender should not
 * use it anymore.
 * If the task failed to be sent, the sender remains the owner of the task.
 */
msg_error_t MSG_task_destroy(msg_task_t task)
{
  if (task->simdata->is_used) {
    /* the task is being sent or executed: cancel it first */
    MSG_task_cancel(task);
  }

  /* free main structures */
  delete task->simdata;
  delete task;

  return MSG_OK;
}

/** @brief Cancel the given task
 *
 * If it was currently executed or transfered, the working process is stopped.
 */
msg_error_t MSG_task_cancel(msg_task_t task)
{
  xbt_assert((task != nullptr), "Cannot cancel a nullptr task");

  simdata_task_t simdata = task->simdata;
  if (simdata->compute) {
    simgrid::simix::simcall([simdata] { simdata->compute->cancel(); });
  } else if (simdata->comm) {
    simdata->comm->cancel();
  }
  simdata->set_not_used();
  return MSG_OK;
}

/** @brief Returns a value in ]0,1[ that represent the task remaining work
 *    to do: starts at 1 and goes to 0. Returns 0 if not started or finished.
 *
 * It works for either parallel or sequential tasks.
 */
double MSG_task_get_remaining_work_ratio(msg_task_t task) {

  xbt_assert((task != nullptr), "Cannot get information from a nullptr task");
  if (task->simdata->compute) {
    // Task in progress
    return task->simdata->compute->get_remaining_ratio();
  } else {
    // Task not started (flops_amount is > 0.0) or finished (flops_amount is set to 0.0)
    return task->simdata->flops_amount > 0.0 ? 1.0 : 0.0;
  }
}

/** @brief Returns the amount of flops that remain to be computed
 *
 * The returned value is initially the cost that you defined for the task, then it decreases until it reaches 0
 *
 * It works for sequential tasks, but the remaining amount of work is not a scalar value for parallel tasks.
 * So you will get an exception if you call this function on parallel tasks. Just don't do it.
 */
double MSG_task_get_flops_amount(msg_task_t task) {
  if (task->simdata->compute != nullptr) {
    return task->simdata->compute->get_remaining();
  } else {
    // Not started or already done.
    // - Before starting, flops_amount is initially the task cost
    // - After execution, flops_amount is set to 0 (until someone uses MSG_task_set_flops_amount, if any)
    return task->simdata->flops_amount;
  }
}

/** @brief set the computation amount needed to process the given task.
 *
 * @warning If the computation is ongoing (already started and not finished),
 * it is not modified by this call. Moreover, after its completion, the ongoing execution with set the flops_amount to
 * zero, overriding any value set during the execution.
 */
void MSG_task_set_flops_amount(msg_task_t task, double flops_amount)
{
  task->simdata->flops_amount = flops_amount;
}

/** @brief set the amount data attached with the given task.
 *
 * @warning If the transfer is ongoing (already started and not finished), it is not modified by this call.
 */
void MSG_task_set_bytes_amount(msg_task_t task, double data_size)
{
  task->simdata->bytes_amount = data_size;
}

/** @brief Returns the total amount received by the given task
 *
 *  If the communication does not exist it will return 0.
 *  So, if the communication has FINISHED or FAILED it returns zero.
 */
double MSG_task_get_remaining_communication(msg_task_t task)
{
  XBT_DEBUG("calling simcall_communication_get_remains(%p)", task->simdata->comm.get());
  return task->simdata->comm->get_remaining();
}

/** @brief Returns the size of the data attached to the given task. */
double MSG_task_get_bytes_amount(msg_task_t task)
{
  xbt_assert((task != nullptr) && (task->simdata != nullptr), "Invalid parameter");
  return task->simdata->bytes_amount;
}

/** @brief Changes the priority of a computation task.
 *
 * This priority doesn't affect the transfer rate. A priority of 2
 * will make a task receive two times more cpu power than regular tasks.
 */
void MSG_task_set_priority(msg_task_t task, double priority)
{
  task->simdata->priority = 1 / priority;
  xbt_assert(std::isfinite(task->simdata->priority), "priority is not finite!");
}

/** @brief Changes the maximum CPU utilization of a computation task (in flops/s).
 *
 * For VMs, there is a pitfall. Please see MSG_vm_set_bound().
 */
void MSG_task_set_bound(msg_task_t task, double bound)
{
  if (bound < 1e-12) /* close enough to 0 without any floating precision surprise */
    XBT_INFO("bound == 0 means no capping (i.e., unlimited).");
  task->simdata->bound = bound;
}
