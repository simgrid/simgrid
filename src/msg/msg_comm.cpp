/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cmath>

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/instr/instr_private.hpp"
#include "src/msg/msg_private.hpp"

namespace simgrid {
namespace msg {

bool Comm::test()
{
  bool finished = false;

  try {
    finished = s_comm->test();
    if (finished && task_received != nullptr) {
      /* I am the receiver */
      (*task_received)->set_not_used();
    }
  } catch (const simgrid::TimeoutException&) {
    status_  = MSG_TIMEOUT;
    finished = true;
  } catch (const simgrid::CancelException&) {
    status_  = MSG_TASK_CANCELED;
    finished = true;
  } catch (const simgrid::NetworkFailureException&) {
    status_  = MSG_TRANSFER_FAILURE;
    finished = true;
  }

  return finished;
}
msg_error_t Comm::wait_for(double timeout)
{
  try {
    s_comm->wait_for(timeout);

    if (task_received != nullptr) {
      /* I am the receiver */
      (*task_received)->set_not_used();
    }

    /* FIXME: these functions are not traceable */
  } catch (const simgrid::TimeoutException&) {
    status_ = MSG_TIMEOUT;
  } catch (const simgrid::CancelException&) {
    status_ = MSG_TASK_CANCELED;
  } catch (const simgrid::NetworkFailureException&) {
    status_ = MSG_TRANSFER_FAILURE;
  }

  return status_;
}
} // namespace msg
} // namespace simgrid

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
  return comm->test();
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
  xbt_dynar_foreach (comms, cursor, comm) {
    s_comms.push_back(static_cast<simgrid::kernel::activity::CommImpl*>(comm->s_comm->get_impl()));
  }

  msg_error_t status = MSG_OK;
  try {
    finished_index = simcall_comm_testany(s_comms.data(), s_comms.size());
  } catch (const simgrid::TimeoutException& e) {
    finished_index = e.value;
    status         = MSG_TIMEOUT;
  } catch (const simgrid::CancelException& e) {
    finished_index = e.value;
    status         = MSG_TASK_CANCELED;
  } catch (const simgrid::NetworkFailureException& e) {
    finished_index = e.value;
    status         = MSG_TRANSFER_FAILURE;
  }

  if (finished_index != -1) {
    comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
    /* the communication is finished */
    comm->set_status(status);

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
  return comm->wait_for(timeout);
}

/** @brief This function is called by a sender and permits waiting for each communication
 *
 * @param comm a vector of communication
 * @param nb_elem is the size of the comm vector
 * @param timeout for each call of MSG_comm_wait
 */
void MSG_comm_waitall(msg_comm_t* comm, int nb_elem, double timeout)
{
  for (int i = 0; i < nb_elem; i++)
    comm[i]->wait_for(timeout);
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
  xbt_dynar_foreach (comms, cursor, comm) {
    s_comms.push_back(static_cast<simgrid::kernel::activity::CommImpl*>(comm->s_comm->get_impl()));
  }

  msg_error_t status = MSG_OK;
  try {
    finished_index = simcall_comm_waitany(s_comms.data(), s_comms.size(), -1);
  } catch (const simgrid::TimeoutException& e) {
    finished_index = e.value;
    status         = MSG_TIMEOUT;
  } catch (const simgrid::CancelException& e) {
    finished_index = e.value;
    status         = MSG_TASK_CANCELED;
  } catch (const simgrid::NetworkFailureException& e) {
    finished_index = e.value;
    status         = MSG_TRANSFER_FAILURE;
  }

  xbt_assert(finished_index != -1, "WaitAny returned -1");

  comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
  /* the communication is finished */
  comm->set_status(status);

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
msg_error_t MSG_comm_get_status(msg_comm_t comm)
{

  return comm->get_status();
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
