/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_PRIVATE_HPP
#define MSG_PRIVATE_HPP

#include "simgrid/msg.h"

#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"

/**************** datatypes **********************************/
/********************************* Task **************************************/

struct s_simdata_task_t {
  ~s_simdata_task_t()
  {
    /* parallel tasks only */
    delete[] host_list;
    delete[] flops_parallel_amount;
    delete[] bytes_parallel_amount;
  }
  void setUsed();
  void setNotUsed() { this->isused = false; }

  simgrid::kernel::activity::ExecImplPtr compute = nullptr; /* SIMIX modeling of computation */
  simgrid::s4u::CommPtr comm                     = nullptr; /* S4U modeling of communication */
  double bytes_amount                            = 0.0;     /* Data size */
  double flops_amount                            = 0.0;     /* Computation size */
  msg_process_t sender                           = nullptr;
  msg_process_t receiver                         = nullptr;
  msg_host_t source                              = nullptr;

  double priority = 1.0;
  double bound    = 0.0; /* Capping for CPU resource, or 0 for no capping */
  double rate     = -1;  /* Capping for network resource, or -1 for no capping*/

  bool isused = false; /* Indicates whether the task is used in SIMIX currently */
  int host_nb = 0;     /* ==0 if sequential task; parallel task if not */
  /*******  Parallel Tasks Only !!!! *******/
  sg_host_t* host_list          = nullptr;
  double* flops_parallel_amount = nullptr;
  double* bytes_parallel_amount = nullptr;

private:
  void reportMultipleUse() const;
};

/******************************* Process *************************************/

namespace simgrid {
namespace msg {

class Comm {
public:
  msg_task_t task_sent;        /* task sent (NULL for the receiver) */
  msg_task_t* task_received;   /* where the task will be received (NULL for the sender) */
  s4u::CommPtr s_comm;         /* SIMIX communication object encapsulated (the same for both processes) */
  msg_error_t status = MSG_OK; /* status of the communication once finished */
  Comm(msg_task_t sent, msg_task_t* received, s4u::CommPtr comm)
      : task_sent(sent), task_received(received), s_comm(std::move(comm))
  {
  }
};
}
}

/************************** Global variables ********************************/
struct s_MSG_Global_t {
  bool debug_multiple_use;           /* whether we want an error message when reusing the same Task for 2 things */
  std::atomic_int_fast32_t sent_msg; /* Total amount of messages sent during the simulation */
  void (*task_copy_callback)(msg_task_t task, msg_process_t src, msg_process_t dst);
  void_f_pvoid_t process_data_cleanup;
};
typedef s_MSG_Global_t* MSG_Global_t;

XBT_PUBLIC_DATA MSG_Global_t msg_global;

/*************************************************************/
XBT_PRIVATE void MSG_comm_copy_data_from_SIMIX(simgrid::kernel::activity::CommImpl* comm, void* buff, size_t buff_size);

/********** Tracing **********/
/* declaration of instrumentation functions from msg_task_instr.c */
XBT_PRIVATE void TRACE_msg_task_put_start(msg_task_t task);


inline void s_simdata_task_t::setUsed()
{
  if (this->isused)
    this->reportMultipleUse();
  this->isused = true;
}

#endif
