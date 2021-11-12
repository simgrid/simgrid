/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/simdag.h>
#include <xbt/Extendable.hpp>

#include <set>
#include <string>
#include <vector>

#ifndef SIMDAG_PRIVATE_HPP
#define SIMDAG_PRIVATE_HPP
#if SIMGRID_HAVE_JEDULE
#include "simgrid/jedule/jedule_sd_binding.h"
#endif

namespace simgrid{
extern template class XBT_PUBLIC xbt::Extendable<sd::Task>;

namespace sd{
class Global;

class Task : public xbt::Extendable<Task> {
  friend sd::Global;

  std::string name_;
  double amount_;

  e_SD_task_kind_t kind_   = SD_TASK_NOT_TYPED;
  e_SD_task_state_t state_ = SD_NOT_SCHEDULED;
  bool marked_             = false; /* used to check if the task DAG has some cycle*/
  double start_time_       = -1;
  double finish_time_      = -1;
  kernel::resource::Action* surf_action_;
  unsigned short watch_points_ = 0; /* bit field xor()ed with masks */
  double rate_                 = -1;

  double alpha_ = 0; /* used by typed parallel tasks */

  /* dependencies */
  std::set<Task*> inputs_;
  std::set<Task*> outputs_;
  std::set<Task*> predecessors_;
  std::set<Task*> successors_;

  /* scheduling parameters (only exist in state SD_SCHEDULED) */
  std::vector<s4u::Host*>* allocation_;
  double* flops_amount_;
  double* bytes_amount_;

protected:
  void set_start_time(double start) { start_time_ = start; }

  void set_sender_side_allocation(unsigned long count, const std::vector<s4u::Host*>* sender);
  void set_receiver_side_allocation(unsigned long count, const std::vector<s4u::Host*>* receiver);

public:
  static Task* create(const std::string& name, double amount, void* userdata);
  static Task* create_comm_e2e(const std::string& name, double amount, void* userdata);
  static Task* create_comp_seq(const std::string& name, double amount, void* userdata);
  static Task* create_comp_par_amdahl(const std::string& name, double amount, void* userdata, double alpha);
  static Task* create_comm_par_mxn_1d_block(const std::string& name, double amount, void* userdata);

  void distribute_comp_amdahl(int count);
  void build_MxN_1D_block_matrix(int src_nb, int dst_nb);

  void add_input(Task* task) { inputs_.insert(task); }
  void rm_input(Task* task) { inputs_.erase(task); }
  void add_predecessor(Task* task) { predecessors_.insert(task); }
  void rm_predecessor(Task* task) { predecessors_.erase(task); }
  void add_successor(Task* task) { successors_.insert(task); }
  void rm_successor(Task* task) { successors_.erase(task); }
  void clear_successors() { successors_.clear(); }
  void add_output(Task* task) { outputs_.insert(task); }
  void rm_output(Task* task) { outputs_.erase(task); }
  void clear_outputs() { outputs_.clear(); }

  void set_name(const std::string& name) { name_ = name; }
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }

  void set_amount(double amount);
  double get_amount() const { return amount_; }
  double get_remaining_amount() const;

  double get_start_time() const;
  double get_finish_time() const;

  void set_state(e_SD_task_state_t new_state);
  e_SD_task_state_t get_state() const { return state_; }

  void mark() { marked_ = true; }
  void unmark() { marked_ = false; }
  bool is_marked() const { return marked_; }

  const std::set<Task*>& get_inputs() const { return inputs_; }
  const std::set<Task*>& get_predecessors() const { return predecessors_; }
  const std::set<Task*>& get_successors() const { return successors_; }
  const std::set<Task*>& get_outputs() const { return outputs_; }

  bool is_parent_of(Task* task) const;
  bool is_child_of(Task* task) const;

  unsigned long has_unsolved_dependencies() const { return (predecessors_.size() + inputs_.size()); }
  unsigned long is_waited_by() const { return (successors_.size() + outputs_.size()); }
  void released_by(Task* pred);
  void produced_by(Task* pred);

  void set_kind(e_SD_task_kind_t kind) { kind_ = kind; }
  e_SD_task_kind_t get_kind() const { return kind_; }

  void set_alpha(double alpha) { alpha_ = alpha; }
  double get_alpha() const;
  void set_rate(double rate);

  unsigned int get_allocation_size() const { return allocation_->size(); }
  std::vector<s4u::Host*>* get_allocation() const { return allocation_; }

  void watch(e_SD_task_state_t state);
  void unwatch(e_SD_task_state_t state);

  void dump() const;

  void do_schedule();
  void schedule(const std::vector<s4u::Host*>& hosts, const double* flops_amount, const double* bytes_amount,
                double rate);
  void schedulev(const std::vector<s4u::Host*>& hosts);
  void unschedule();

  void run();
  void destroy();
};

class Global {
public:
  explicit Global(int* argc, char** argv) : engine_(new simgrid::s4u::Engine(argc, argv)) {}
  bool watch_point_reached = false; /* has a task just reached a watch point? */
  std::set<Task*> initial_tasks;
  std::set<Task*> runnable_tasks;
  std::set<Task*> completed_tasks;
  std::set<Task*> return_set;
  s4u::Engine* engine_;
};

} // namespace sd
} // namespace simgrid

extern XBT_PRIVATE std::unique_ptr<simgrid::sd::Global> sd_global;

/* SimDag private functions */
XBT_PRIVATE bool acyclic_graph_detail(const_xbt_dynar_t dag);
XBT_PRIVATE void uniq_transfer_task_name(SD_task_t task);
XBT_PRIVATE const char *__get_state_name(e_SD_task_state_t state);
#endif
