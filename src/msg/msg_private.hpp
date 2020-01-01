/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_PRIVATE_HPP
#define MSG_PRIVATE_HPP

#include "simgrid/Exception.hpp"
#include "simgrid/msg.h"
#include "src/kernel/activity/CommImpl.hpp"
#include <simgrid/modelchecker.h>
#include <xbt/Extendable.hpp>

#include <cmath>

/**************** datatypes **********************************/
namespace simgrid {
namespace msg {
class Task : public xbt::Extendable<Task> {
  std::string name_             = "";
  std::string tracing_category_ = "";
  long long int id_;

  double timeout_  = 0.0;
  double priority_ = 1.0;
  double bound_    = 0.0;   /* Capping for CPU resource, or 0 for no capping */
  double rate_     = -1;    /* Capping for network resource, or -1 for no capping*/
  bool is_used_    = false; /* Indicates whether the task is used in SIMIX currently */

  explicit Task(const std::string& name, double flops_amount, double bytes_amount, void* data);
  explicit Task(const std::string& name, std::vector<s4u::Host*>&& hosts, std::vector<double>&& flops_amount,
                std::vector<double>&& bytes_amount, void* data);

  void report_multiple_use() const;

public:
  static Task* create(const std::string& name, double flops_amount, double bytes_amount, void* data);
  static Task* create_parallel(const std::string& name, int host_nb, const msg_host_t* host_list, double* flops_amount,
                               double* bytes_amount, void* data);
  msg_error_t execute();
  msg_error_t send(const std::string& alias, double timeout);
  s4u::CommPtr send_async(const std::string& alias, void_f_pvoid_t cleanup, bool detached);

  void cancel();

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;
  ~Task()                      = default;

  bool is_used() const { return is_used_; }
  bool is_parallel() const { return parallel_; }

  void set_used();
  void set_not_used() { this->is_used_ = false; }
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  void set_name(const char* new_name) { name_ = std::string(new_name); }
  void set_tracing_category(const char* category) { tracing_category_ = category ? std::string(category) : ""; }
  const std::string& get_tracing_category() const { return tracing_category_; }
  bool has_tracing_category() { return not tracing_category_.empty(); }
  XBT_ATTRIB_DEPRECATED_v329("Please use set_data()") void* get_user_data() { return get_data(); }
  XBT_ATTRIB_DEPRECATED_v329("Please use get_data()") void set_user_data(void* data) { set_data(data); }
  long long int get_id() const { return id_; }
  double get_priority() const { return priority_; }
  void set_priority(double priority);
  void set_bound(double bound) { bound_ = bound; }
  double get_bound() const { return bound_; }
  void set_rate(double rate) { rate_ = rate; }
  double get_rate() const { return rate_; }
  void set_timeout(double timeout) { timeout_ = timeout; }

  s4u::Actor* get_sender() const;
  s4u::Host* get_source() const;

  s4u::ExecPtr compute = nullptr; /* S4U modeling of computation */
  s4u::CommPtr comm    = nullptr; /* S4U modeling of communication */
  double flops_amount  = 0.0;     /* Computation size */
  double bytes_amount  = 0.0;     /* Data size */

  /*******  Parallel Tasks Only !!!! *******/
  bool parallel_ = false;
  std::vector<s4u::Host*> hosts_;
  std::vector<double> flops_parallel_amount;
  std::vector<double> bytes_parallel_amount;
};

class Comm {
  msg_error_t status_ = MSG_OK; /* status of the communication once finished */
public:
  Task* task_sent;             /* task sent (NULL for the receiver) */
  Task** task_received;        /* where the task will be received (NULL for the sender) */
  s4u::CommPtr s_comm;         /* SIMIX communication object encapsulated (the same for both processes) */
  Comm(msg_task_t sent, msg_task_t* received, s4u::CommPtr comm)
      : task_sent(sent), task_received(received), s_comm(std::move(comm))
  {
  }
  bool test();
  msg_error_t wait_for(double timeout);
  void set_status(msg_error_t status) { status_ = status; }
  msg_error_t get_status() const { return status_; }
};

} // namespace msg
} // namespace simgrid

/************************** Global variables ********************************/
struct MSG_Global_t {
  static bool debug_multiple_use;    /* whether we want an error message when reusing the same Task for 2 things */
  std::atomic_int_fast32_t sent_msg; /* Total amount of messages sent during the simulation */
  void (*task_copy_callback)(msg_task_t task, msg_process_t src, msg_process_t dst);
  void_f_pvoid_t process_data_cleanup;
};

XBT_PUBLIC_DATA MSG_Global_t* msg_global;

/*************************************************************/

#endif
