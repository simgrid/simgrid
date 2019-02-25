/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_PRIVATE_HPP
#define MSG_PRIVATE_HPP

#include "simgrid/msg.h"

#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"

/**************** datatypes **********************************/
namespace simgrid {
namespace msg {
class Task {
  std::string name_             = "";
  std::string tracing_category_ = "";

public:
  ~Task();
  explicit Task(std::string name, double flops_amount, double bytes_amount)
      : name_(std::move(name)), flops_amount(flops_amount), bytes_amount(bytes_amount)
  {
  }
  void set_used();
  void set_not_used() { this->is_used = false; }

  const std::string& get_name() const { return name_; }
  const char* get_cname() { return name_.c_str(); }
  void set_name(const char* new_name) { name_ = std::string(new_name); }
  void set_tracing_category(const char* category) { tracing_category_ = category ? std::string(category) : ""; }
  const std::string& get_tracing_category() { return tracing_category_; }
  bool has_tracing_category() { return not tracing_category_.empty(); }

  kernel::activity::ExecImplPtr compute          = nullptr; /* SIMIX modeling of computation */
  s4u::CommPtr comm                              = nullptr; /* S4U modeling of communication */
  double flops_amount                            = 0.0;     /* Computation size */
  double bytes_amount                            = 0.0;     /* Data size */
  msg_process_t sender                           = nullptr;
  msg_process_t receiver                         = nullptr;

  double priority = 1.0;
  double bound    = 0.0; /* Capping for CPU resource, or 0 for no capping */
  double rate     = -1;  /* Capping for network resource, or -1 for no capping*/

  bool is_used = false; /* Indicates whether the task is used in SIMIX currently */
  int host_nb = 0;     /* ==0 if sequential task; parallel task if not */
  /*******  Parallel Tasks Only !!!! *******/
  sg_host_t* host_list          = nullptr;
  double* flops_parallel_amount = nullptr;
  double* bytes_parallel_amount = nullptr;

private:
  void report_multiple_use() const;
};

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

} // namespace msg
} // namespace simgrid

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


#endif
