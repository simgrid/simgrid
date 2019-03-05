/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cmath>

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/instr/instr_private.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/msg/msg_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_gos, msg, "Logging specific to MSG (gos)");

/**
 * @brief Executes a parallel task and waits for its termination.
 *
 * @param task a #msg_task_t to execute on the location on which the process is running.
 *
 * @return #MSG_OK if the task was successfully completed, #MSG_TASK_CANCELED or #MSG_HOST_FAILURE otherwise
 */
msg_error_t MSG_parallel_task_execute(msg_task_t task)
{
  return MSG_parallel_task_execute_with_timeout(task, -1);
}

msg_error_t MSG_parallel_task_execute_with_timeout(msg_task_t task, double timeout)
{
  e_smx_state_t comp_state;
  msg_error_t status = MSG_OK;

  xbt_assert((not task->compute) && not task->is_used(), "This task is executed somewhere else. Go fix your code!");

  XBT_DEBUG("Computing on %s", MSG_process_get_name(MSG_process_self()));

  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::by_name(instr_pid(MSG_process_self()))->get_state("ACTOR_STATE")->push_event("execute");

  try {
    task->set_used();

    task->compute = boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(simcall_execution_parallel_start(
        std::move(task->get_name()), task->hosts_.size(), task->hosts_.data(),
        (task->flops_parallel_amount.empty() ? nullptr : task->flops_parallel_amount.data()),
        (task->bytes_parallel_amount.empty() ? nullptr : task->bytes_parallel_amount.data()), -1.0, timeout));
    XBT_DEBUG("Parallel execution action created: %p", task->compute.get());
    if (task->has_tracing_category())
      simgrid::simix::simcall([task] { task->compute->set_category(std::move(task->get_tracing_category())); });

    comp_state = simcall_execution_wait(task->compute);

    task->set_not_used();

    XBT_DEBUG("Execution task '%s' finished in state %d", task->get_cname(), (int)comp_state);
  } catch (simgrid::HostFailureException& e) {
    status = MSG_HOST_FAILURE;
  } catch (simgrid::TimeoutError& e) {
    status = MSG_TIMEOUT;
  } catch (simgrid::CancelException& e) {
    status = MSG_TASK_CANCELED;
  }

  /* action ended, set comm and compute = nullptr, the actions is already destroyed in the main function */
  task->flops_amount = 0.0;
  task->comm         = nullptr;
  task->compute      = nullptr;

  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::by_name(instr_pid(MSG_process_self()))->get_state("ACTOR_STATE")->pop_event();

  return status;
}

/**
 * @brief Receives a task from a mailbox.
 *
 * This is a blocking function, the execution flow will be blocked until the task is received. See #MSG_task_irecv
 * for receiving tasks asynchronously.
 *
 * @param task a memory location for storing a #msg_task_t.
 * @param alias name of the mailbox to receive the task from
 *
 * @return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_receive(msg_task_t * task, const char *alias)
{
  return MSG_task_receive_with_timeout(task, alias, -1);
}

/**
 * @brief Receives a task from a mailbox at a given rate.
 *
 * @param task a memory location for storing a #msg_task_t.
 * @param alias name of the mailbox to receive the task from
 * @param rate limit the reception to rate bandwidth (byte/sec)
 *
 * The rate parameter can be used to receive a task with a limited bandwidth (smaller than the physical available
 * value). Use MSG_task_receive() if you don't limit the rate (or pass -1 as a rate value do disable this feature).
 *
 * @return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_receive_bounded(msg_task_t * task, const char *alias, double rate)
{
  return MSG_task_receive_with_timeout_bounded(task, alias, -1, rate);
}

/**
 * @brief Receives a task from a mailbox with a given timeout.
 *
 * This is a blocking function with a timeout, the execution flow will be blocked until the task is received or the
 * timeout is achieved. See #MSG_task_irecv for receiving tasks asynchronously.  You can provide a -1 timeout
 * to obtain an infinite timeout.
 *
 * @param task a memory location for storing a #msg_task_t.
 * @param alias name of the mailbox to receive the task from
 * @param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_receive)
 *
 * @return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_receive_with_timeout(msg_task_t * task, const char *alias, double timeout)
{
  return MSG_task_receive_ext(task, alias, timeout, nullptr);
}

/**
 * @brief Receives a task from a mailbox with a given timeout and at a given rate.
 *
 * @param task a memory location for storing a #msg_task_t.
 * @param alias name of the mailbox to receive the task from
 * @param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_receive)
 * @param rate limit the reception to rate bandwidth (byte/sec)
 *
 * The rate parameter can be used to send a task with a limited
 * bandwidth (smaller than the physical available value). Use
 * MSG_task_receive() if you don't limit the rate (or pass -1 as a
 * rate value do disable this feature).
 *
 * @return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_receive_with_timeout_bounded(msg_task_t* task, const char* alias, double timeout, double rate)
{
  return MSG_task_receive_ext_bounded(task, alias, timeout, nullptr, rate);
}

/**
 * @brief Receives a task from a mailbox from a specific host with a given timeout.
 *
 * This is a blocking function with a timeout, the execution flow will be blocked until the task is received or the
 * timeout is achieved. See #MSG_task_irecv for receiving tasks asynchronously. You can provide a -1 timeout
 * to obtain an infinite timeout.
 *
 * @param task a memory location for storing a #msg_task_t.
 * @param alias name of the mailbox to receive the task from
 * @param timeout is the maximum wait time for completion (provide -1 for no timeout)
 * @param host a #msg_host_t host from where the task was sent
 *
 * @return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_receive_ext(msg_task_t * task, const char *alias, double timeout, msg_host_t host)
{
  XBT_DEBUG("MSG_task_receive_ext: Trying to receive a message on mailbox '%s'", alias);
  return MSG_task_receive_ext_bounded(task, alias, timeout, host, -1.0);
}

/**
 * @brief Receives a task from a mailbox from a specific host with a given timeout  and at a given rate.
 *
 * @param task a memory location for storing a #msg_task_t.
 * @param alias name of the mailbox to receive the task from
 * @param timeout is the maximum wait time for completion (provide -1 for no timeout)
 * @param host a #msg_host_t host from where the task was sent
 * @param rate limit the reception to rate bandwidth (byte/sec)
 *
 * The rate parameter can be used to receive a task with a limited bandwidth (smaller than the physical available
 * value). Use MSG_task_receive_ext() if you don't limit the rate (or pass -1 as a rate value do disable this feature).
 *
 * @return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_receive_ext_bounded(msg_task_t * task, const char *alias, double timeout, msg_host_t host,
                                         double rate)
{
  XBT_DEBUG("MSG_task_receive_ext: Trying to receive a message on mailbox '%s'", alias);
  msg_error_t ret = MSG_OK;
  /* We no longer support getting a task from a specific host */
  if (host)
    THROW_UNIMPLEMENTED;

  /* Sanity check */
  xbt_assert(task, "Null pointer for the task storage");

  if (*task)
    XBT_WARN("Asked to write the received task in a non empty struct -- proceeding.");

  /* Try to receive it by calling SIMIX network layer */
  try {
    void* payload;
    simgrid::s4u::Mailbox::by_name(alias)
        ->get_init()
        ->set_dst_data(&payload, sizeof(msg_task_t*))
        ->set_rate(rate)
        ->wait_for(timeout);
    *task = static_cast<msg_task_t>(payload);
    XBT_DEBUG("Got task %s from %s", (*task)->get_cname(), alias);
    (*task)->set_not_used();
  } catch (simgrid::HostFailureException& e) {
    ret = MSG_HOST_FAILURE;
  } catch (simgrid::TimeoutError& e) {
    ret = MSG_TIMEOUT;
  } catch (simgrid::CancelException& e) {
    ret = MSG_TASK_CANCELED;
  } catch (xbt_ex& e) {
    if (e.category == network_error)
      ret = MSG_TRANSFER_FAILURE;
    else
      throw;
  }

  if (TRACE_actor_is_enabled() && ret != MSG_HOST_FAILURE && ret != MSG_TRANSFER_FAILURE && ret != MSG_TIMEOUT) {
    container_t process_container = simgrid::instr::Container::by_name(instr_pid(MSG_process_self()));

    std::string key = std::string("p") + std::to_string((*task)->get_id());
    simgrid::instr::Container::get_root()->get_link("ACTOR_TASK_LINK")->end_event(process_container, "SR", key);
  }
  return ret;
}


/**
 * @brief Starts listening for receiving a task from an asynchronous communication.
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test() to end the communication.
 *
 * @param task a memory location for storing a #msg_task_t. has to be valid until the end of the communication.
 * @param name of the mailbox to receive the task on
 * @return the msg_comm_t communication created
 */
msg_comm_t MSG_task_irecv(msg_task_t *task, const char *name)
{
  return MSG_task_irecv_bounded(task, name, -1.0);
}

/**
 * @brief Starts listening for receiving a task from an asynchronous communication at a given rate.
 *
 * The rate parameter can be used to receive a task with a limited
 * bandwidth (smaller than the physical available value). Use
 * MSG_task_irecv() if you don't limit the rate (or pass -1 as a rate
 * value do disable this feature).
 *
 * @param task a memory location for storing a #msg_task_t. has to be valid until the end of the communication.
 * @param name of the mailbox to receive the task on
 * @param rate limit the bandwidth to the given rate (byte/sec)
 * @return the msg_comm_t communication created
 */
msg_comm_t MSG_task_irecv_bounded(msg_task_t *task, const char *name, double rate)
{
  simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::by_name(name);

  /* FIXME: these functions are not traceable */
  /* Sanity check */
  xbt_assert(task, "Null pointer for the task storage");

  if (*task)
    XBT_CRITICAL("MSG_task_irecv() was asked to write in a non empty task struct.");

  /* Try to receive it by calling SIMIX network layer */
  simgrid::s4u::CommPtr comm = simgrid::s4u::Mailbox::by_name(name)
                                   ->get_init()
                                   ->set_dst_data((void**)task, sizeof(msg_task_t*))
                                   ->set_rate(rate)
                                   ->start();

  return new simgrid::msg::Comm(nullptr, task, comm);
}

/**
 * @brief Checks whether a communication is done, and if yes, finalizes it.
 * @param comm the communication to test
 * @return 'true' if the communication is finished
 * (but it may have failed, use MSG_comm_get_status() to know its status)
 * or 'false' if the communication is not finished yet
 * If the status is 'false', don't forget to use MSG_process_sleep() after the test.
 */
int MSG_comm_test(msg_comm_t comm)
{
  bool finished = false;

  try {
    finished = comm->s_comm->test();
    if (finished && comm->task_received != nullptr) {
      /* I am the receiver */
      (*comm->task_received)->set_not_used();
    }
  } catch (simgrid::TimeoutError& e) {
    comm->status = MSG_TIMEOUT;
    finished     = true;
  } catch (simgrid::CancelException& e) {
    comm->status = MSG_TASK_CANCELED;
    finished     = true;
  }
  catch (xbt_ex& e) {
    if (e.category == network_error) {
      comm->status = MSG_TRANSFER_FAILURE;
      finished     = true;
    } else {
      throw;
    }
  }

  return finished;
}

/**
 * @brief This function checks if a communication is finished.
 * @param comms a vector of communications
 * @return the position of the finished communication if any
 * (but it may have failed, use MSG_comm_get_status() to know its status), or -1 if none is finished
 */
int MSG_comm_testany(xbt_dynar_t comms)
{
  int finished_index = -1;

  /* Create the equivalent array with SIMIX objects: */
  std::vector<simgrid::kernel::activity::CommImpl*> s_comms;
  s_comms.reserve(xbt_dynar_length(comms));
  msg_comm_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(comms, cursor, comm) {
    s_comms.push_back(static_cast<simgrid::kernel::activity::CommImpl*>(comm->s_comm->get_impl().get()));
  }

  msg_error_t status = MSG_OK;
  try {
    finished_index = simcall_comm_testany(s_comms.data(), s_comms.size());
  } catch (simgrid::TimeoutError& e) {
    finished_index = e.value;
    status         = MSG_TIMEOUT;
  } catch (simgrid::CancelException& e) {
    finished_index = e.value;
    status         = MSG_TASK_CANCELED;
  }
  catch (xbt_ex& e) {
    if (e.category != network_error)
      throw;
    finished_index = e.value;
    status         = MSG_TRANSFER_FAILURE;
  }

  if (finished_index != -1) {
    comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
    /* the communication is finished */
    comm->status = status;

    if (status == MSG_OK && comm->task_received != nullptr) {
      /* I am the receiver */
      (*comm->task_received)->set_not_used();
    }
  }

  return finished_index;
}

/** @brief Destroys the provided communication. */
void MSG_comm_destroy(msg_comm_t comm)
{
  delete comm;
}

/** @brief Wait for the completion of a communication.
 *
 * It takes two parameters.
 * @param comm the communication to wait.
 * @param timeout Wait until the communication terminates or the timeout occurs.
 *                You can provide a -1 timeout to obtain an infinite timeout.
 * @return msg_error_t
 */
msg_error_t MSG_comm_wait(msg_comm_t comm, double timeout)
{
  try {
    comm->s_comm->wait_for(timeout);

    if (comm->task_received != nullptr) {
      /* I am the receiver */
      (*comm->task_received)->set_not_used();
    }

    /* FIXME: these functions are not traceable */
  } catch (simgrid::TimeoutError& e) {
    comm->status = MSG_TIMEOUT;
  } catch (simgrid::CancelException& e) {
    comm->status = MSG_TASK_CANCELED;
  }
  catch (xbt_ex& e) {
    if (e.category == network_error)
      comm->status = MSG_TRANSFER_FAILURE;
    else
      throw;
  }

  return comm->status;
}

/** @brief This function is called by a sender and permit to wait for each communication
 *
 * @param comm a vector of communication
 * @param nb_elem is the size of the comm vector
 * @param timeout for each call of MSG_comm_wait
 */
void MSG_comm_waitall(msg_comm_t * comm, int nb_elem, double timeout)
{
  for (int i = 0; i < nb_elem; i++)
    MSG_comm_wait(comm[i], timeout);
}

/** @brief This function waits for the first communication finished in a list.
 * @param comms a vector of communications
 * @return the position of the first finished communication
 * (but it may have failed, use MSG_comm_get_status() to know its status)
 */
int MSG_comm_waitany(xbt_dynar_t comms)
{
  int finished_index = -1;

  /* Create the equivalent array with SIMIX objects: */
  std::vector<simgrid::kernel::activity::CommImpl*> s_comms;
  s_comms.reserve(xbt_dynar_length(comms));
  msg_comm_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(comms, cursor, comm) {
    s_comms.push_back(static_cast<simgrid::kernel::activity::CommImpl*>(comm->s_comm->get_impl().get()));
  }

  msg_error_t status = MSG_OK;
  try {
    finished_index = simcall_comm_waitany(s_comms.data(), s_comms.size(), -1);
  } catch (simgrid::TimeoutError& e) {
    finished_index = e.value;
    status         = MSG_TIMEOUT;
  } catch (simgrid::CancelException& e) {
    finished_index = e.value;
    status         = MSG_TASK_CANCELED;
  }
  catch(xbt_ex& e) {
    if (e.category == network_error) {
      finished_index = e.value;
      status         = MSG_TRANSFER_FAILURE;
    } else {
      throw;
    }
  }

  xbt_assert(finished_index != -1, "WaitAny returned -1");

  comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
  /* the communication is finished */
  comm->status = status;

  if (comm->task_received != nullptr) {
    /* I am the receiver */
    (*comm->task_received)->set_not_used();
  }

  return finished_index;
}

/**
 * @brief Returns the error (if any) that occurred during a finished communication.
 * @param comm a finished communication
 * @return the status of the communication, or #MSG_OK if no error occurred during the communication
 */
msg_error_t MSG_comm_get_status(msg_comm_t comm) {

  return comm->status;
}

/** @brief Get a task (#msg_task_t) from a communication
 *
 * @param comm the communication where to get the task
 * @return the task from the communication
 */
msg_task_t MSG_comm_get_task(msg_comm_t comm)
{
  xbt_assert(comm, "Invalid parameter");

  return comm->task_received ? *comm->task_received : comm->task_sent;
}

/**
 * @brief This function is called by SIMIX in kernel mode to copy the data of a comm.
 * @param comm the comm
 * @param buff the data copied
 * @param buff_size size of the buffer
 */
void MSG_comm_copy_data_from_SIMIX(simgrid::kernel::activity::CommImpl* comm, void* buff, size_t buff_size)
{
  SIMIX_comm_copy_pointer_callback(comm, buff, buff_size);

  // notify the user callback if any
  if (msg_global->task_copy_callback) {
    msg_task_t task = static_cast<msg_task_t>(buff);
    msg_global->task_copy_callback(task, comm->src_actor_->ciface(), comm->dst_actor_->ciface());
  }
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
msg_error_t MSG_task_send(msg_task_t task, const char *alias)
{
  XBT_DEBUG("MSG_task_send: Trying to send a message on mailbox '%s'", alias);
  return MSG_task_send_with_timeout(task, alias, -1);
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
msg_error_t MSG_task_send_bounded(msg_task_t task, const char *alias, double maxrate)
{
  task->set_rate(maxrate);
  return MSG_task_send(task, alias);
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
msg_error_t MSG_task_send_with_timeout(msg_task_t task, const char *alias, double timeout)
{
  msg_error_t ret = MSG_OK;
  /* Try to send it */
  try {
    simgrid::s4u::CommPtr comm = task->send_async(alias, nullptr, false);
    task->comm = comm;
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
    task->set_not_used();
  }

  return ret;
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
msg_error_t MSG_task_send_with_timeout_bounded(msg_task_t task, const char *alias, double timeout, double maxrate)
{
  task->set_rate(maxrate);
  return MSG_task_send_with_timeout(task, alias, timeout);
}

/**
 * @brief Look if there is a communication on a mailbox and return the PID of the sender process.
 *
 * @param alias the name of the mailbox to be considered
 *
 * @return Returns the PID of sender process,
 * -1 if there is no communication in the mailbox.#include <cmath>
 *
 */
int MSG_task_listen_from(const char *alias)
{
  /* looks inside the rdv directly. Not clean. */
  simgrid::kernel::activity::CommImplPtr comm = simgrid::s4u::Mailbox::by_name(alias)->front();

  if (comm && comm->src_actor_)
    return comm->src_actor_->get_pid();
  else
    return -1;
}
