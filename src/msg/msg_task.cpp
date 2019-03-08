/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.hpp"
#include "src/instr/instr_private.hpp"
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/Mailbox.hpp>

#include <algorithm>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_task, msg, "Logging specific to MSG (task)");

namespace simgrid {
namespace msg {

Task::Task(std::string name, double flops_amount, double bytes_amount, void* data)
    : name_(std::move(name)), userdata_(data), flops_amount(flops_amount), bytes_amount(bytes_amount)
{
  static std::atomic_ullong counter{0};
  id_ = counter++;
  if (MC_is_active())
    MC_ignore_heap(&(id_), sizeof(id_));
}

Task::Task(std::string name, std::vector<s4u::Host*> hosts, std::vector<double> flops_amount,
           std::vector<double> bytes_amount, void* data)
    : Task(std::move(name), 1.0, 0, data)
{
  parallel_             = true;
  hosts_                = std::move(hosts);
  flops_parallel_amount = std::move(flops_amount);
  bytes_parallel_amount = std::move(bytes_amount);
}

Task* Task::create(std::string name, double flops_amount, double bytes_amount, void* data)
{
  return new Task(std::move(name), flops_amount, bytes_amount, data);
}

Task* Task::create_parallel(std::string name, int host_nb, const msg_host_t* host_list, double* flops_amount,
                            double* bytes_amount, void* data)
{
  std::vector<s4u::Host*> hosts;
  std::vector<double> flops;
  std::vector<double> bytes;

  for (int i = 0; i < host_nb; i++) {
    hosts.push_back(host_list[i]);
    if (flops_amount != nullptr)
      flops.push_back(flops_amount[i]);
    if (bytes_amount != nullptr) {
      for (int j = 0; j < host_nb; j++)
        bytes.push_back(bytes_amount[host_nb * i + j]);
    }
  }
  return new Task(std::move(name), std::move(hosts), std::move(flops), std::move(bytes), data);
}

msg_error_t Task::execute()
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(flops_amount), "flops_amount is not finite!");

  msg_error_t status = MSG_OK;
  if (flops_amount <= 0.0)
    return MSG_OK;

  try {
    set_used();
    if (parallel_)
      compute = s4u::this_actor::exec_init(hosts_, flops_parallel_amount, bytes_parallel_amount);
    else
      compute = s4u::this_actor::exec_init(flops_amount);

    compute->set_name(name_)
        ->set_tracing_category(tracing_category_)
        ->set_timeout(timeout_)
        ->set_priority(1 / priority_)
        ->set_bound(bound_)
        ->wait();

    set_not_used();
    XBT_DEBUG("Execution task '%s' finished", get_cname());
  } catch (HostFailureException& e) {
    status = MSG_HOST_FAILURE;
  } catch (TimeoutError& e) {
    status = MSG_TIMEOUT;
  } catch (CancelException& e) {
    status = MSG_TASK_CANCELED;
  }

  /* action ended, set comm and compute = nullptr, the actions is already destroyed in the main function */
  flops_amount = 0.0;
  comm         = nullptr;
  compute      = nullptr;

  return status;
}

s4u::CommPtr Task::send_async(std::string alias, void_f_pvoid_t cleanup, bool detached)
{
  if (TRACE_actor_is_enabled()) {
    container_t process_container = simgrid::instr::Container::by_name(instr_pid(MSG_process_self()));
    std::string key               = std::string("p") + std::to_string(get_id());
    simgrid::instr::Container::get_root()->get_link("ACTOR_TASK_LINK")->start_event(process_container, "SR", key);
  }

  /* Prepare the task to send */
  set_used();
  this->comm = nullptr;
  msg_global->sent_msg++;

  s4u::CommPtr s4u_comm = s4u::Mailbox::by_name(alias)->put_init(this, bytes_amount)->set_rate(get_rate());
  comm                  = s4u_comm;

  if (detached)
    comm->detach(cleanup);
  else
    comm->start();

  if (TRACE_is_enabled() && has_tracing_category())
    simgrid::simix::simcall([this] { comm->get_impl()->set_category(std::move(tracing_category_)); });

  return comm;
}

msg_error_t Task::send(std::string alias, double timeout)
{
  msg_error_t ret = MSG_OK;
  /* Try to send it */
  try {
    comm = nullptr; // needed, otherwise MC gets confused.
    s4u::CommPtr s4u_comm = send_async(alias, nullptr, false);
    comm                  = s4u_comm;
    comm->wait_for(timeout);
  } catch (simgrid::TimeoutError& e) {
    ret = MSG_TIMEOUT;
  } catch (simgrid::CancelException& e) {
    ret = MSG_HOST_FAILURE;
  } catch (xbt_ex& e) {
    if (e.category == network_error)
      ret = MSG_TRANSFER_FAILURE;
    else
      throw;

    /* If the send failed, it is not used anymore */
    set_not_used();
  }

  return ret;
}
void Task::cancel()
{
  if (compute) {
    compute->cancel();
  } else if (comm) {
    comm->cancel();
  }
  set_not_used();
}

void Task::set_priority(double priority)
{
  xbt_assert(std::isfinite(1.0 / priority), "priority is not finite!");
  priority_ = 1.0 / priority;
}

s4u::Actor* Task::get_sender()
{
  return comm ? comm->get_sender().get() : nullptr;
}

s4u::Host* Task::get_source()
{
  return comm ? comm->get_sender()->get_host() : nullptr;
}

void Task::set_used()
{
  if (is_used_)
    report_multiple_use();
  is_used_ = true;
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
  return simgrid::msg::Task::create(name ? std::string(name) : "", flop_amount, message_size, data);
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
  return simgrid::msg::Task::create_parallel(name ? name : "", host_nb, host_list, flops_amount, bytes_amount, data);
}

/** @brief Return the user data of the given task */
void* MSG_task_get_data(msg_task_t task)
{
  return task->get_user_data();
}

/** @brief Sets the user data of a given task */
void MSG_task_set_data(msg_task_t task, void *data)
{
  task->set_user_data(data);
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
  return task->get_sender();
}

/** @brief Returns the source (the sender's host) of the given task */
msg_host_t MSG_task_get_source(msg_task_t task)
{
  return task->get_source();
}

/** @brief Returns the name of the given task. */
const char *MSG_task_get_name(msg_task_t task)
{
  return task->get_cname();
}

/** @brief Sets the name of the given task. */
void MSG_task_set_name(msg_task_t task, const char *name)
{
  task->set_name(name);
}

/**
 * @brief Executes a task and waits for its termination.
 *
 * This function is used for describing the behavior of a process. It takes only one parameter.
 * @param task a #msg_task_t to execute on the location on which the process is running.
 * @return #MSG_OK if the task was successfully completed, #MSG_TASK_CANCELED or #MSG_HOST_FAILURE otherwise
 */
msg_error_t MSG_task_execute(msg_task_t task)
{
  return task->execute();
}

/**
 * @brief Executes a parallel task and waits for its termination.
 *
 * @param task a #msg_task_t to execute on the location on which the process is running.
 *
 * @return #MSG_OK if the task was successfully completed, #MSG_TASK_CANCELED or #MSG_HOST_FAILURE otherwise
 */
msg_error_t MSG_parallel_task_execute(msg_task_t task)
{
  return task->execute();
}

msg_error_t MSG_parallel_task_execute_with_timeout(msg_task_t task, double timeout)
{
  task->set_timeout(timeout);
  return task->execute();
}

/**
 * @brief Sends a task on a mailbox.
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test() to end the communication.
 *
 * @param task a #msg_task_t to send on another location.
 * @param alias name of the mailbox to sent the task to
 * @return the msg_comm_t communication created
 */
msg_comm_t MSG_task_isend(msg_task_t task, const char* alias)
{
  return new simgrid::msg::Comm(task, nullptr, task->send_async(alias, nullptr, false));
}

/**
 * @brief Sends a task on a mailbox with a maximum rate
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test() to end the communication. The maxrate
 * parameter allows the application to limit the bandwidth utilization of network links when sending the task.
 *
 * @param task a #msg_task_t to send on another location.
 * @param alias name of the mailbox to sent the task to
 * @param maxrate the maximum communication rate for sending this task (byte/sec).
 * @return the msg_comm_t communication created
 */
msg_comm_t MSG_task_isend_bounded(msg_task_t task, const char* alias, double maxrate)
{
  task->set_rate(maxrate);
  return new simgrid::msg::Comm(task, nullptr, task->send_async(alias, nullptr, false));
}

/**
 * @brief Sends a task on a mailbox.
 *
 * This is a non blocking detached send function.
 * Think of it as a best effort send. Keep in mind that the third parameter is only called if the communication fails.
 * If the communication does work, it is responsibility of the receiver code to free anything related to the task, as
 * usual. More details on this can be obtained on
 * <a href="http://lists.gforge.inria.fr/pipermail/simgrid-user/2011-November/002649.html">this thread</a>
 * in the SimGrid-user mailing list archive.
 *
 * @param task a #msg_task_t to send on another location.
 * @param alias name of the mailbox to sent the task to
 * @param cleanup a function to destroy the task if the communication fails, e.g. MSG_task_destroy
 * (if nullptr, no function will be called)
 */
void MSG_task_dsend(msg_task_t task, const char* alias, void_f_pvoid_t cleanup)
{
  task->send_async(alias, cleanup, true);
}

/**
 * @brief Sends a task on a mailbox with a maximal rate.
 *
 * This is a non blocking detached send function.
 * Think of it as a best effort send. Keep in mind that the third parameter is only called if the communication fails.
 * If the communication does work, it is responsibility of the receiver code to free anything related to the task, as
 * usual. More details on this can be obtained on
 * <a href="http://lists.gforge.inria.fr/pipermail/simgrid-user/2011-November/002649.html">this thread</a>
 * in the SimGrid-user mailing list archive.
 *
 * The rate parameter can be used to send a task with a limited bandwidth (smaller than the physical available value).
 * Use MSG_task_dsend() if you don't limit the rate (or pass -1 as a rate value do disable this feature).
 *
 * @param task a #msg_task_t to send on another location.
 * @param alias name of the mailbox to sent the task to
 * @param cleanup a function to destroy the task if the communication fails, e.g. MSG_task_destroy (if nullptr, no
 *        function will be called)
 * @param maxrate the maximum communication rate for sending this task (byte/sec)
 *
 */
void MSG_task_dsend_bounded(msg_task_t task, const char* alias, void_f_pvoid_t cleanup, double maxrate)
{
  task->set_rate(maxrate);
  task->send_async(alias, cleanup, true);
}
/**
 * @brief Sends a task to a mailbox
 *
 * This is a blocking function, the execution flow will be blocked until the task is sent (and received on the other
 * side if #MSG_task_receive is used).
 * See #MSG_task_isend for sending tasks asynchronously.
 *
 * @param task the task to be sent
 * @param alias the mailbox name to where the task is sent
 *
 * @return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_send(msg_task_t task, const char* alias)
{
  XBT_DEBUG("MSG_task_send: Trying to send a message on mailbox '%s'", alias);
  return task->send(alias, -1);
}

/**
 * @brief Sends a task to a mailbox with a maximum rate
 *
 * This is a blocking function, the execution flow will be blocked until the task is sent. The maxrate parameter allows
 * the application to limit the bandwidth utilization of network links when sending the task.
 *
 * The maxrate parameter can be used to send a task with a limited bandwidth (smaller than the physical available
 * value). Use MSG_task_send() if you don't limit the rate (or pass -1 as a rate value do disable this feature).
 *
 * @param task the task to be sent
 * @param alias the mailbox name to where the task is sent
 * @param maxrate the maximum communication rate for sending this task (byte/sec)
 *
 * @return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_send_bounded(msg_task_t task, const char* alias, double maxrate)
{
  task->set_rate(maxrate);
  return task->send(alias, -1);
}

/**
 * @brief Sends a task to a mailbox with a timeout
 *
 * This is a blocking function, the execution flow will be blocked until the task is sent or the timeout is achieved.
 *
 * @param task the task to be sent
 * @param alias the mailbox name to where the task is sent
 * @param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_send)
 *
 * @return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_send_with_timeout(msg_task_t task, const char* alias, double timeout)
{
  return task->send(alias, timeout);
}

/**
 * @brief Sends a task to a mailbox with a timeout and with a maximum rate
 *
 * This is a blocking function, the execution flow will be blocked until the task is sent or the timeout is achieved.
 *
 * The maxrate parameter can be used to send a task with a limited bandwidth (smaller than the physical available
 * value). Use MSG_task_send_with_timeout() if you don't limit the rate (or pass -1 as a rate value do disable this
 * feature).
 *
 * @param task the task to be sent
 * @param alias the mailbox name to where the task is sent
 * @param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_send)
 * @param maxrate the maximum communication rate for sending this task (byte/sec)
 *
 * @return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_send_with_timeout_bounded(msg_task_t task, const char* alias, double timeout, double maxrate)
{
  task->set_rate(maxrate);
  return task->send(alias, timeout);
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
  if (task->is_used()) {
    /* the task is being sent or executed: cancel it first */
    task->cancel();
  }

  /* free main structures */
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
  task->cancel();
  return MSG_OK;
}

/** @brief Returns a value in ]0,1[ that represent the task remaining work
 *    to do: starts at 1 and goes to 0. Returns 0 if not started or finished.
 *
 * It works for either parallel or sequential tasks.
 */
double MSG_task_get_remaining_work_ratio(msg_task_t task) {

  xbt_assert((task != nullptr), "Cannot get information from a nullptr task");
  if (task->compute) {
    // Task in progress
    return task->compute->get_remaining_ratio();
  } else {
    // Task not started (flops_amount is > 0.0) or finished (flops_amount is set to 0.0)
    return task->flops_amount > 0.0 ? 1.0 : 0.0;
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
  if (task->compute != nullptr) {
    return task->compute->get_remaining();
  } else {
    // Not started or already done.
    // - Before starting, flops_amount is initially the task cost
    // - After execution, flops_amount is set to 0 (until someone uses MSG_task_set_flops_amount, if any)
    return task->flops_amount;
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
  task->flops_amount = flops_amount;
}

/** @brief set the amount data attached with the given task.
 *
 * @warning If the transfer is ongoing (already started and not finished), it is not modified by this call.
 */
void MSG_task_set_bytes_amount(msg_task_t task, double data_size)
{
  task->bytes_amount = data_size;
}

/** @brief Returns the total amount received by the given task
 *
 *  If the communication does not exist it will return 0.
 *  So, if the communication has FINISHED or FAILED it returns zero.
 */
double MSG_task_get_remaining_communication(msg_task_t task)
{
  XBT_DEBUG("calling simcall_communication_get_remains(%p)", task->comm.get());
  return task->comm->get_remaining();
}

/** @brief Returns the size of the data attached to the given task. */
double MSG_task_get_bytes_amount(msg_task_t task)
{
  xbt_assert(task != nullptr, "Invalid parameter");
  return task->bytes_amount;
}

/** @brief Changes the priority of a computation task.
 *
 * This priority doesn't affect the transfer rate. A priority of 2
 * will make a task receive two times more cpu power than regular tasks.
 */
void MSG_task_set_priority(msg_task_t task, double priority)
{
  task->set_priority(priority);
}

/** @brief Changes the maximum CPU utilization of a computation task (in flops/s).
 *
 * For VMs, there is a pitfall. Please see MSG_vm_set_bound().
 */
void MSG_task_set_bound(msg_task_t task, double bound)
{
  if (bound < 1e-12) /* close enough to 0 without any floating precision surprise */
    XBT_INFO("bound == 0 means no capping (i.e., unlimited).");
  task->set_bound(bound);
}

/**
 * @brief Sets the tracing category of a task.
 *
 * This function should be called after the creation of a MSG task, to define the category of that task. The
 * first parameter task must contain a task that was  =created with the function #MSG_task_create. The second
 * parameter category must contain a category that was previously declared with the function #TRACE_category
 * (or with #TRACE_category_with_color).
 *
 * See @ref outcomes_vizu for details on how to trace the (categorized) resource utilization.
 *
 * @param task the task that is going to be categorized
 * @param category the name of the category to be associated to the task
 *
 * @see MSG_task_get_category, TRACE_category, TRACE_category_with_color
 */
void MSG_task_set_category(msg_task_t task, const char* category)
{
  xbt_assert(not task->has_tracing_category(), "Task %p(%s) already has a category (%s).", task, task->get_cname(),
             task->get_tracing_category().c_str());

  // if user provides a nullptr category, task is no longer traced
  if (category == nullptr) {
    task->set_tracing_category("");
    XBT_DEBUG("MSG task %p(%s), category removed", task, task->get_cname());
  } else {
    // set task category
    task->set_tracing_category(category);
    XBT_DEBUG("MSG task %p(%s), category %s", task, task->get_cname(), task->get_tracing_category().c_str());
  }
}

/**
 * @brief Gets the current tracing category of a task. (@see MSG_task_set_category)
 * @param task the task to be considered
 * @return Returns the name of the tracing category of the given task, "" otherwise
 */
const char* MSG_task_get_category(msg_task_t task)
{
  return task->get_tracing_category().c_str();
}
