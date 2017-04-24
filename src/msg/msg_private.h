/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef METASIMGRID_PRIVATE_H
#define METASIMGRID_PRIVATE_H

#include "simgrid/msg.h"

#include "src/kernel/activity/SynchroExec.hpp"
#include "src/kernel/activity/SynchroComm.hpp"

#include <xbt/Extendable.hpp>

/**************** datatypes **********************************/
/**************************** Host Extension *********************************/
namespace simgrid {
class MsgHostExt {
public:
  static simgrid::xbt::Extension<s4u::Host, MsgHostExt> EXTENSION_ID;

  ~MsgHostExt() {
    delete file_descriptor_table;
  }
  std::vector<int>* file_descriptor_table = nullptr; // Created lazily on need
};
}
/********************************* Task **************************************/

typedef struct simdata_task {
  ~simdata_task()
  {
    if (this->compute)
      this->compute->unref();

    /* parallel tasks only */
    xbt_free(this->host_list);
  }
  void setUsed();
  void setNotUsed()
  {
    this->isused = false;
  }

  simgrid::kernel::activity::Exec *compute = nullptr; /* SIMIX modeling of computation */
  simgrid::kernel::activity::Comm *comm = nullptr;    /* SIMIX modeling of communication */
  double bytes_amount = 0.0; /* Data size */
  double flops_amount = 0.0; /* Computation size */
  msg_process_t sender = nullptr;
  msg_process_t receiver = nullptr;
  msg_host_t source = nullptr;

  double priority = 1.0;
  double bound    = 0.0; /* Capping for CPU resource, or 0 for no capping */
  double rate     = -1;  /* Capping for network resource, or -1 for no capping*/

  bool isused = false;  /* Indicates whether the task is used in SIMIX currently */
  int host_nb = 0;      /* ==0 if sequential task; parallel task if not */
  /*******  Parallel Tasks Only !!!! *******/
  sg_host_t *host_list = nullptr;
  double *flops_parallel_amount = nullptr;
  double *bytes_parallel_amount = nullptr;

private:
  void reportMultipleUse() const;
} s_simdata_task_t;

/********************************* File **************************************/
typedef struct simdata_file {
  smx_file_t smx_file;
} s_simdata_file_t;

/******************************* Process *************************************/

namespace simgrid {
class MsgActorExt {
public:
  explicit MsgActorExt(void* d) : data(d) {}
  msg_error_t errno_ = MSG_OK;  /* the last value returned by a MSG_function */
  void* data = nullptr; /* user data */
};
}

typedef struct msg_comm {
  smx_activity_t s_comm;          /* SIMIX communication object encapsulated (the same for both processes) */
  msg_task_t task_sent;           /* task sent (NULL for the receiver) */
  msg_task_t *task_received;      /* where the task will be received (NULL for the sender) */
  msg_error_t status;           /* status of the communication once finished */
} s_msg_comm_t;

/************************** Global variables ********************************/
typedef struct MSG_Global {
  int debug_multiple_use;       /* whether we want an error message when reusing the same Task for 2 things */
  unsigned long int sent_msg;   /* Total amount of messages sent during the simulation */
  void (*task_copy_callback) (msg_task_t task, msg_process_t src, msg_process_t dst);
  void_f_pvoid_t process_data_cleanup;
} s_MSG_Global_t;
typedef s_MSG_Global_t* MSG_Global_t;

SG_BEGIN_DECL()

XBT_PUBLIC_DATA(MSG_Global_t) msg_global;

/*************************************************************/

XBT_PRIVATE msg_host_t __MSG_host_create(sg_host_t host);
XBT_PRIVATE msg_storage_t __MSG_storage_create(smx_storage_t storage);
XBT_PRIVATE void __MSG_storage_destroy(msg_storage_priv_t host);
XBT_PRIVATE void __MSG_file_destroy(msg_file_t file);

XBT_PRIVATE void MSG_process_cleanup_from_SIMIX(smx_actor_t smx_proc);
XBT_PRIVATE smx_actor_t MSG_process_create_from_SIMIX(const char* name, std::function<void()> code, void* data,
                                                      sg_host_t host, xbt_dict_t properties,
                                                      smx_actor_t parent_process);
XBT_PRIVATE void MSG_comm_copy_data_from_SIMIX(smx_activity_t comm, void* buff, size_t buff_size);

XBT_PRIVATE void MSG_post_create_environment();

XBT_PRIVATE void MSG_host_add_task(msg_host_t host, msg_task_t task);
XBT_PRIVATE void MSG_host_del_task(msg_host_t host, msg_task_t task);

/********** Tracing **********/
/* declaration of instrumentation functions from msg_task_instr.c */
XBT_PRIVATE void TRACE_msg_set_task_category(msg_task_t task, const char *category);
XBT_PRIVATE void TRACE_msg_task_create(msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_execute_start(msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_execute_end(msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_destroy(msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_get_end(double start_time, msg_task_t task);
XBT_PRIVATE void TRACE_msg_task_get_start();
XBT_PRIVATE int TRACE_msg_task_put_start(msg_task_t task);    //returns TRUE if the task_put_end must be called
XBT_PRIVATE void TRACE_msg_task_put_end();

/* declaration of instrumentation functions from msg_process_instr.c */
XBT_PRIVATE char *instr_process_id (msg_process_t proc, char *str, int len);
XBT_PRIVATE char *instr_process_id_2 (const char *process_name, int process_pid, char *str, int len);
XBT_PRIVATE void TRACE_msg_process_change_host(msg_process_t process, msg_host_t old_host, msg_host_t new_host);
XBT_PRIVATE void TRACE_msg_process_create (const char *process_name, int process_pid, msg_host_t host);
XBT_PRIVATE void TRACE_msg_process_destroy (const char *process_name, int process_pid);
XBT_PRIVATE void TRACE_msg_process_kill(smx_process_exit_status_t status, msg_process_t process);
XBT_PRIVATE void TRACE_msg_process_suspend(msg_process_t process);
XBT_PRIVATE void TRACE_msg_process_resume(msg_process_t process);
XBT_PRIVATE void TRACE_msg_process_sleep_in(msg_process_t process);   //called from msg/gos.c
XBT_PRIVATE void TRACE_msg_process_sleep_out(msg_process_t process);

SG_END_DECL()

inline void simdata_task::setUsed()
{
  if (this->isused)
    this->reportMultipleUse();
  if (msg_global->debug_multiple_use) {
    // TODO, backtrace
  }
  this->isused = true;
}

#endif
