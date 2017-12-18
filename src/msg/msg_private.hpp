/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_PRIVATE_HPP
#define MSG_PRIVATE_HPP

#include "simgrid/msg.h"

#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "xbt/Extendable.hpp"

#include <atomic>

/**************** datatypes **********************************/
/********************************* Task **************************************/

struct s_simdata_task_t {
  ~s_simdata_task_t()
  {
    /* parallel tasks only */
    delete[] this->host_list;
    /* flops_parallel_amount and bytes_parallel_amount are automatically deleted in ~L07Action */
  }
  void setUsed();
  void setNotUsed() { this->isused = false; }

  simgrid::kernel::activity::ExecImplPtr compute = nullptr; /* SIMIX modeling of computation */
  simgrid::kernel::activity::CommImplPtr comm    = nullptr; /* SIMIX modeling of communication */
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
class ActorExt {
public:
  explicit ActorExt(void* d) : data(d) {}
  msg_error_t errno_ = MSG_OK;  /* the last value returned by a MSG_function */
  void* data         = nullptr; /* user data */
};

class Comm {
public:
  msg_task_t task_sent;        /* task sent (NULL for the receiver) */
  msg_task_t* task_received;   /* where the task will be received (NULL for the sender) */
  smx_activity_t s_comm;       /* SIMIX communication object encapsulated (the same for both processes) */
  msg_error_t status = MSG_OK; /* status of the communication once finished */
  Comm(msg_task_t sent, msg_task_t* received, smx_activity_t comm)
      : task_sent(sent), task_received(received), s_comm(std::move(comm))
  {
  }
};
}
}

/************************** Global variables ********************************/
struct s_MSG_Global_t {
  int debug_multiple_use;            /* whether we want an error message when reusing the same Task for 2 things */
  std::atomic_int_fast32_t sent_msg; /* Total amount of messages sent during the simulation */
  void (*task_copy_callback)(msg_task_t task, msg_process_t src, msg_process_t dst);
  void_f_pvoid_t process_data_cleanup;
};
typedef s_MSG_Global_t* MSG_Global_t;

XBT_PRIVATE std::string instr_pid(msg_process_t proc);
XBT_PRIVATE void TRACE_msg_process_create(std::string process_name, int process_pid, msg_host_t host);
XBT_PRIVATE void TRACE_msg_process_destroy(std::string process_name, int process_pid);

extern "C" {

XBT_PUBLIC_DATA(MSG_Global_t) msg_global;

/*************************************************************/
XBT_PRIVATE void MSG_process_cleanup_from_SIMIX(smx_actor_t smx_proc);
XBT_PRIVATE smx_actor_t MSG_process_create_from_SIMIX(const char* name, std::function<void()> code, void* data,
                                                      sg_host_t host, std::map<std::string, std::string>* properties,
                                                      smx_actor_t parent_process);
XBT_PRIVATE void MSG_comm_copy_data_from_SIMIX(smx_activity_t comm, void* buff, size_t buff_size);

/********** Tracing **********/
/* declaration of instrumentation functions from msg_task_instr.c */
XBT_PRIVATE void TRACE_msg_set_task_category(msg_task_t task, const char* category);
XBT_PRIVATE void TRACE_msg_task_create(msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_execute_start(msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_execute_end(msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_destroy(msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_get_end(msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_get_start();
XBT_PRIVATE int TRACE_msg_task_put_start(msg_task_t task); // returns TRUE if the task_put_end must be called
XBT_PRIVATE void TRACE_msg_task_put_end();

/* declaration of instrumentation functions from msg_process_instr.c */
XBT_PRIVATE void TRACE_msg_process_change_host(msg_process_t process, msg_host_t new_host);
XBT_PRIVATE void TRACE_msg_process_kill(smx_process_exit_status_t status, msg_process_t process);
XBT_PRIVATE void TRACE_msg_process_suspend(msg_process_t process);
XBT_PRIVATE void TRACE_msg_process_resume(msg_process_t process);
XBT_PRIVATE void TRACE_msg_process_sleep_in(msg_process_t process); // called from msg/gos.c
XBT_PRIVATE void TRACE_msg_process_sleep_out(msg_process_t process);
}

inline void s_simdata_task_t::setUsed()
{
  if (this->isused)
    this->reportMultipleUse();
  this->isused = true;
}

#endif
