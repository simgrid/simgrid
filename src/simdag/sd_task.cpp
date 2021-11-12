/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simdag_private.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/surf_interface.hpp"
#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_task, sd, "Logging specific to SimDag (task)");

namespace simgrid {

template class xbt::Extendable<sd::Task>;

namespace sd {

Task* Task::create(const std::string& name, double amount, void* userdata)
{
  auto task = new Task();
  task->set_name(name);
  task->set_amount(amount);
  task->set_data(userdata);
  task->allocation_ = new std::vector<sg_host_t>();
  sd_global->initial_tasks.insert(task);

  return task;
}

Task* Task::create_comm_e2e(const std::string& name, double amount, void* userdata)
{
  auto task              = create(name, amount, userdata);
  task->bytes_amount_    = xbt_new0(double, 4);
  task->bytes_amount_[2] = amount;
  task->set_kind(SD_TASK_COMM_E2E);

  return task;
}

Task* Task::create_comp_seq(const std::string& name, double amount, void* userdata)
{
  auto task              = create(name, amount, userdata);
  task->flops_amount_    = xbt_new0(double, 1);
  task->flops_amount_[0] = amount;
  task->set_kind(SD_TASK_COMP_SEQ);

  return task;
}

Task* Task::create_comp_par_amdahl(const std::string& name, double amount, void* userdata, double alpha)
{
  xbt_assert(alpha < 1. && alpha >= 0., "Invalid parameter: alpha must be in [0.;1.[");

  auto task = create(name, amount, userdata);
  task->set_alpha(alpha);
  task->set_kind(SD_TASK_COMP_PAR_AMDAHL);

  return task;
}

Task* Task::create_comm_par_mxn_1d_block(const std::string& name, double amount, void* userdata)
{
  auto task = create(name, amount, userdata);
  task->set_kind(SD_TASK_COMM_PAR_MXN_1D_BLOCK);

  return task;
}

void Task::distribute_comp_amdahl(int count)
{
  xbt_assert(kind_ == SD_TASK_COMP_PAR_AMDAHL,
             "Task %s is not a SD_TASK_COMP_PAR_AMDAHL typed task."
             "Cannot use this function.",
             get_cname());
  flops_amount_ = xbt_new0(double, count);
  for (int i = 0; i < count; i++)
    flops_amount_[i] = (alpha_ + (1 - alpha_) / count) * amount_;
}

void Task::build_MxN_1D_block_matrix(int src_nb, int dst_nb)
{
  xbt_assert(kind_ == SD_TASK_COMM_PAR_MXN_1D_BLOCK,
             "Task %s is not a SD_TASK_COMM_PAR_MXN_1D_BLOCK typed task."
             "Cannot use this function.",
             get_cname());
  xbt_free(bytes_amount_);
  bytes_amount_ = xbt_new0(double, allocation_->size() * allocation_->size());

  for (int i = 0; i < src_nb; i++) {
    double src_start = i * amount_ / src_nb;
    double src_end   = src_start + amount_ / src_nb;
    for (int j = 0; j < dst_nb; j++) {
      double dst_start = j * amount_ / dst_nb;
      double dst_end   = dst_start + amount_ / dst_nb;
      XBT_VERB("(%d->%d): (%.2f, %.2f)-> (%.2f, %.2f)", i, j, src_start, src_end, dst_start, dst_end);
      bytes_amount_[i * (src_nb + dst_nb) + src_nb + j] = 0.0;
      if ((src_end > dst_start) && (dst_end > src_start)) { /* There is something to send */
        bytes_amount_[i * (src_nb + dst_nb) + src_nb + j] = std::min(src_end, dst_end) - std::max(src_start, dst_start);
        XBT_VERB("==> %.2f", bytes_amount_[i * (src_nb + dst_nb) + src_nb + j]);
      }
    }
  }
}

bool Task::is_parent_of(Task* task) const
{
  return (successors_.find(task) != successors_.end() || outputs_.find(task) != outputs_.end());
}

bool Task::is_child_of(Task* task) const
{
  return (inputs_.find(task) != inputs_.end() || predecessors_.find(task) != predecessors_.end());
}

void Task::set_amount(double amount)
{
  amount_ = amount;
  if (kind_ == SD_TASK_COMP_SEQ)
    flops_amount_[0] = amount;
  if (kind_ == SD_TASK_COMM_E2E) {
    bytes_amount_[2] = amount;
  }
}

void Task::set_rate(double rate)
{
  xbt_assert(kind_ == SD_TASK_COMM_E2E, "The rate can be modified for end-to-end communications only.");
  if (state_ < SD_RUNNING) {
    rate_ = rate;
  } else {
    XBT_WARN("Task %p has started. Changing rate is ineffective.", this);
  }
}
void Task::set_state(e_SD_task_state_t new_state)
{
  std::set<Task*>::iterator idx;
  XBT_DEBUG("Set state of '%s' to %d", get_cname(), new_state);
  if ((new_state == SD_NOT_SCHEDULED || new_state == SD_SCHEDULABLE) && state_ == SD_FAILED) {
    sd_global->completed_tasks.erase(this);
    sd_global->initial_tasks.insert(this);
  }

  if (new_state == SD_SCHEDULED && state_ == SD_RUNNABLE) {
    sd_global->initial_tasks.insert(this);
    sd_global->runnable_tasks.erase(this);
  }

  if (new_state == SD_RUNNABLE) {
    idx = sd_global->initial_tasks.find(this);
    if (idx != sd_global->initial_tasks.end()) {
      sd_global->runnable_tasks.insert(*idx);
      sd_global->initial_tasks.erase(idx);
    }
  }

  if (new_state == SD_RUNNING)
    sd_global->runnable_tasks.erase(this);

  if (new_state == SD_DONE || new_state == SD_FAILED) {
    sd_global->completed_tasks.insert(this);
    start_time_ = surf_action_->get_start_time();
    if (new_state == SD_DONE) {
      finish_time_ = surf_action_->get_finish_time();
#if SIMGRID_HAVE_JEDULE
      jedule_log_sd_event(this);
#endif
    } else
      finish_time_ = simgrid_get_clock();
    surf_action_->unref();
    surf_action_ = nullptr;
    allocation_->clear();
  }

  state_ = new_state;

  if (watch_points_ & new_state) {
    XBT_VERB("Watch point reached with task '%s'!", get_cname());
    sd_global->watch_point_reached = true;
    unwatch(new_state); /* remove the watch point */
  }
}

double Task::get_alpha() const
{
  xbt_assert(kind_ == SD_TASK_COMP_PAR_AMDAHL, "Alpha parameter is not defined for this kind of task");
  return alpha_;
}

double Task::get_remaining_amount() const
{
  if (surf_action_)
    return surf_action_->get_remains();
  else
    return (state_ == SD_DONE) ? 0 : amount_;
}

double Task::get_start_time() const
{
  if (surf_action_)
    return surf_action_->get_start_time();
  else
    return start_time_;
}

double Task::get_finish_time() const
{
  if (surf_action_) /* should never happen as actions are destroyed right after their completion */
    return surf_action_->get_finish_time();
  else
    return finish_time_;
}

void Task::set_sender_side_allocation(unsigned long count, const std::vector<s4u::Host*>* sender)
{
  for (unsigned long i = 0; i < count; i++)
    allocation_->push_back(sender->at(i));
}

void Task::set_receiver_side_allocation(unsigned long count, const std::vector<s4u::Host*>* receiver)
{
  for (unsigned long i = 0; i < count; i++)
    allocation_->insert(allocation_->begin() + i, receiver->at(i));
}

void Task::watch(e_SD_task_state_t state)
{
  if (state & SD_NOT_SCHEDULED)
    throw std::invalid_argument("Cannot add a watch point for state SD_NOT_SCHEDULED");

  watch_points_ = watch_points_ | state;
}

void Task::unwatch(e_SD_task_state_t state)
{
  xbt_assert(state != SD_NOT_SCHEDULED, "SimDag error: Cannot have a watch point for state SD_NOT_SCHEDULED");
  watch_points_ = watch_points_ & ~state;
}

void Task::dump() const
{
  XBT_INFO("Displaying task %s", get_cname());
  if (state_ == SD_RUNNABLE)
    XBT_INFO("  - state: runnable");
  else if (state_ < SD_RUNNABLE)
    XBT_INFO("  - state: %s not runnable", __get_state_name(state_));
  else
    XBT_INFO("  - state: not runnable %s", __get_state_name(state_));

  if (kind_ != 0) {
    switch (kind_) {
      case SD_TASK_COMM_E2E:
        XBT_INFO("  - kind: end-to-end communication");
        break;
      case SD_TASK_COMP_SEQ:
        XBT_INFO("  - kind: sequential computation");
        break;
      case SD_TASK_COMP_PAR_AMDAHL:
        XBT_INFO("  - kind: parallel computation following Amdahl's law");
        break;
      case SD_TASK_COMM_PAR_MXN_1D_BLOCK:
        XBT_INFO("  - kind: MxN data redistribution assuming 1D block distribution");
        break;
      default:
        XBT_INFO("  - (unknown kind %d)", kind_);
    }
  }

  XBT_INFO("  - amount: %.0f", amount_);
  if (kind_ == SD_TASK_COMP_PAR_AMDAHL)
    XBT_INFO("  - alpha: %.2f", alpha_);
  XBT_INFO("  - Dependencies to satisfy: %zu", has_unsolved_dependencies());
  if (has_unsolved_dependencies() > 0) {
    XBT_INFO("  - pre-dependencies:");
    for (auto const& it : predecessors_)
      XBT_INFO("    %s", it->get_cname());

    for (auto const& it : inputs_)
      XBT_INFO("    %s", it->get_cname());
  }
  if ((outputs_.size() + successors_.size()) > 0) {
    XBT_INFO("  - post-dependencies:");

    for (auto const& it : successors_)
      XBT_INFO("    %s", it->get_cname());
    for (auto const& it : outputs_)
      XBT_INFO("    %s", it->get_cname());
  }
}

void Task::released_by(Task* pred)
{
  predecessors_.erase(pred);
  inputs_.erase(pred);
  XBT_DEBUG("Release dependency on %s: %zu remain(s). Becomes schedulable if %zu=0", get_cname(),
            has_unsolved_dependencies(), predecessors_.size());

  if (state_ == SD_NOT_SCHEDULED && predecessors_.empty())
    set_state(SD_SCHEDULABLE);

  if (state_ == SD_SCHEDULED && has_unsolved_dependencies() == 0)
    set_state(SD_RUNNABLE);

  if (state_ == SD_RUNNABLE && not sd_global->watch_point_reached)
    run();
}

void Task::produced_by(Task* pred)
{
  start_time_ = pred->get_finish_time();
  predecessors_.erase(pred);
  if (state_ == SD_SCHEDULED)
    set_state(SD_RUNNABLE);
  else
    set_state(SD_SCHEDULABLE);

  Task* comm_dst = *(successors_.begin());
  if (comm_dst->get_state() == SD_NOT_SCHEDULED && comm_dst->get_predecessors().empty()) {
    XBT_DEBUG("%s is a transfer, %s may be ready now if %zu=0", get_cname(), comm_dst->get_cname(),
              comm_dst->get_predecessors().size());
    comm_dst->set_state(SD_SCHEDULABLE);
  }
  if (state_ == SD_RUNNABLE && not sd_global->watch_point_reached)
    run();
}

void Task::do_schedule()
{
  if (state_ > SD_SCHEDULABLE)
    throw std::invalid_argument(simgrid::xbt::string_printf("Task '%s' has already been scheduled", get_cname()));

  if (has_unsolved_dependencies() == 0)
    set_state(SD_RUNNABLE);
  else
    set_state(SD_SCHEDULED);
}

void Task::schedule(const std::vector<s4u::Host*>& hosts, const double* flops_amount, const double* bytes_amount,
                    double rate)
{
  unsigned long host_count = hosts.size();
  rate_                    = rate;

  if (flops_amount) {
    flops_amount_ = static_cast<double*>(xbt_realloc(flops_amount_, sizeof(double) * host_count));
    memcpy(flops_amount_, flops_amount, sizeof(double) * host_count);
  } else {
    xbt_free(flops_amount_);
    flops_amount_ = nullptr;
  }

  unsigned long communication_nb = host_count * host_count;
  if (bytes_amount) {
    bytes_amount_ = static_cast<double*>(xbt_realloc(bytes_amount_, sizeof(double) * communication_nb));
    memcpy(bytes_amount_, bytes_amount, sizeof(double) * communication_nb);
  } else {
    xbt_free(bytes_amount_);
    bytes_amount_ = nullptr;
  }

  for (unsigned long i = 0; i < host_count; i++)
    allocation_->push_back(hosts[i]);

  do_schedule();
}

void Task::schedulev(const std::vector<s4u::Host*>& hosts)
{
  xbt_assert(kind_ == SD_TASK_COMP_SEQ || kind_ == SD_TASK_COMP_PAR_AMDAHL,
             "Task %s is not typed. Cannot automatically schedule it.", get_cname());

  for (unsigned long i = 0; i < hosts.size(); i++)
    allocation_->push_back(hosts[i]);

  XBT_VERB("Schedule computation task %s on %zu host(s)", get_cname(), allocation_->size());

  if (kind_ == SD_TASK_COMP_SEQ) {
    if (not flops_amount_) { /*This task has failed and is rescheduled. Reset the flops_amount*/
      flops_amount_    = xbt_new0(double, 1);
      flops_amount_[0] = amount_;
    }
    XBT_VERB("It costs %.f flops", flops_amount_[0]);
  }

  if (kind_ == SD_TASK_COMP_PAR_AMDAHL) {
    distribute_comp_amdahl(hosts.size());
    XBT_VERB("%.f flops will be distributed following Amdahl's Law", flops_amount_[0]);
  }

  do_schedule();

  /* Iterate over all inputs and outputs to say where I am located (and start them if runnable) */
  for (auto const& input : inputs_) {
    unsigned long src_nb = input->get_allocation_size();
    unsigned long dst_nb = hosts.size();
    if (src_nb == 0)
      XBT_VERB("Sender side of '%s' not scheduled. Set receiver side to '%s''s allocation", input->get_cname(),
               get_cname());
    input->set_sender_side_allocation(dst_nb, allocation_);

    if (input->get_allocation_size() > allocation_->size()) {
      if (kind_ == SD_TASK_COMP_PAR_AMDAHL)
        input->build_MxN_1D_block_matrix(src_nb, dst_nb);

      input->do_schedule();
      XBT_VERB("Auto-Schedule Communication task '%s'. Send %.f bytes from %zu hosts to %zu hosts.", input->get_cname(),
               input->get_amount(), src_nb, dst_nb);
    }
  }

  for (auto const& output : outputs_) {
    unsigned long src_nb = hosts.size();
    unsigned long dst_nb = output->get_allocation_size();
    if (dst_nb == 0)
      XBT_VERB("Receiver side of '%s' not scheduled. Set sender side to '%s''s allocation", output->get_cname(),
               get_cname());
    output->set_receiver_side_allocation(src_nb, allocation_);

    if (output->get_allocation_size() > allocation_->size()) {
      if (kind_ == SD_TASK_COMP_PAR_AMDAHL)
        output->build_MxN_1D_block_matrix(src_nb, dst_nb);

      output->do_schedule();
      XBT_VERB("Auto-Schedule Communication task %s. Send %.f bytes from %zu hosts to %zu hosts.", output->get_cname(),
               output->get_amount(), src_nb, dst_nb);
    }
  }
}

void Task::unschedule()
{
  if (state_ == SD_NOT_SCHEDULED || state_ == SD_SCHEDULABLE)
    throw std::invalid_argument(xbt::string_printf(
        "Task %s: the state must be SD_SCHEDULED, SD_RUNNABLE, SD_RUNNING or SD_FAILED", get_cname()));

  if (state_ == SD_SCHEDULED || state_ == SD_RUNNABLE) /* if the task is scheduled or runnable */ {
    allocation_->clear();
    if (kind_ == SD_TASK_COMP_PAR_AMDAHL || kind_ == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
      /* Don't free scheduling data for typed tasks */
      xbt_free(flops_amount_);
      xbt_free(bytes_amount_);
      bytes_amount_ = nullptr;
      flops_amount_ = nullptr;
    }
  }

  if (state_ == SD_RUNNING)
    /* the task should become SD_FAILED */
    surf_action_->cancel();
  else {
    if (has_unsolved_dependencies() == 0)
      set_state(SD_SCHEDULABLE);
    else
      set_state(SD_NOT_SCHEDULED);
  }
  start_time_ = -1.0;
}

void Task::run()
{
  xbt_assert(state_ == SD_RUNNABLE, "Task '%s' is not runnable! Task state: %d", get_cname(), (int)state_);
  xbt_assert(not allocation_->empty(), "Task '%s': host_list is empty!", get_cname());

  XBT_VERB("Executing task '%s'", get_cname());

  /* Beware! The scheduling data are now used by the surf action directly! no copy was done */
  auto host_model = allocation_->front()->get_netpoint()->get_englobing_zone()->get_host_model();
  surf_action_    = host_model->execute_parallel(*allocation_, flops_amount_, bytes_amount_, rate_);

  surf_action_->set_data(this);

  XBT_DEBUG("surf_action = %p", surf_action_);

  set_state(SD_RUNNING);
  sd_global->return_set.insert(this);
}

void Task::destroy()
{
  XBT_DEBUG("Destroying task %s...", get_cname());

  /* First Remove all dependencies associated with the task. */
  while (not predecessors_.empty())
    SD_task_dependency_remove(*(predecessors_.begin()), this);
  while (not inputs_.empty())
    SD_task_dependency_remove(*(inputs_.begin()), this);
  while (not successors_.empty())
    SD_task_dependency_remove(this, *(successors_.begin()));
  while (not outputs_.empty())
    SD_task_dependency_remove(this, *(outputs_.begin()));

  if (state_ == SD_SCHEDULED || state_ == SD_RUNNABLE) {
    xbt_free(flops_amount_);
    xbt_free(bytes_amount_);
    bytes_amount_ = nullptr;
    flops_amount_ = nullptr;
  }

  xbt_free(flops_amount_);
  xbt_free(bytes_amount_);

  delete allocation_;

  if (surf_action_ != nullptr)
    surf_action_->unref();

  XBT_DEBUG("Task destroyed.");
}
} // namespace sd
} // namespace simgrid

/* **************************** Public C interface *************************** */

/**
 * @brief Creates a new task.
 *
 * @param name the name of the task (can be @c nullptr)
 * @param data the user data you want to associate with the task (can be @c nullptr)
 * @param amount amount of the task
 * @return the new task
 * @see SD_task_destroy()
 */
SD_task_t SD_task_create(const char* name, void* data, double amount)
{
  return simgrid::sd::Task::create(name, amount, data);
}

/** @brief create an end-to-end communication task that can then be auto-scheduled
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows one to specify the task costs at
 * creation, and decouple them from the scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * A end-to-end communication must be scheduled on 2 hosts, and the amount specified at creation is sent from hosts[0]
 * to hosts[1].
 */
SD_task_t SD_task_create_comm_e2e(const char* name, void* data, double amount)
{
  return simgrid::sd::Task::create_comm_e2e(name, amount, data);
}

/** @brief create a sequential computation task that can then be auto-scheduled
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows one to specify the task costs at
 * creation, and decouple them from the scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * A sequential computation must be scheduled on 1 host, and the amount specified at creation to be run on hosts[0].
 *
 * @param name the name of the task (can be @c nullptr)
 * @param data the user data you want to associate with the task (can be @c nullptr)
 * @param flops_amount amount of compute work to be done by the task
 * @return the new SD_TASK_COMP_SEQ typed task
 */
SD_task_t SD_task_create_comp_seq(const char* name, void* data, double flops_amount)
{
  return simgrid::sd::Task::create_comp_seq(name, flops_amount, data);
}

/** @brief create a parallel computation task that can then be auto-scheduled
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows one to specify the task costs at
 * creation, and decouple them from the scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * A parallel computation can be scheduled on any number of host.
 * The underlying speedup model is Amdahl's law.
 * To be auto-scheduled, @see SD_task_distribute_comp_amdahl has to be called first.
 * @param name the name of the task (can be @c nullptr)
 * @param data the user data you want to associate with the task (can be @c nullptr)
 * @param flops_amount amount of compute work to be done by the task
 * @param alpha purely serial fraction of the work to be done (in [0.;1.[)
 * @return the new task
 */
SD_task_t SD_task_create_comp_par_amdahl(const char* name, void* data, double flops_amount, double alpha)
{
  return simgrid::sd::Task::create_comp_par_amdahl(name, flops_amount, data, alpha);
}

/** @brief create a complex data redistribution task that can then be  auto-scheduled
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev().
 * This allows one to specify the task costs at creation, and decouple them from the scheduling process where you just
 * specify which resource should communicate.
 *
 * A data redistribution can be scheduled on any number of host.
 * The assumed distribution is a 1D block distribution. Each host owns the same share of the @see amount.
 * To be auto-scheduled, @see SD_task_distribute_comm_mxn_1d_block has to be  called first.
 * @param name the name of the task (can be @c nullptr)
 * @param data the user data you want to associate with the task (can be @c nullptr)
 * @param amount amount of data to redistribute by the task
 * @return the new task
 */
SD_task_t SD_task_create_comm_par_mxn_1d_block(const char* name, void* data, double amount)
{
  return simgrid::sd::Task::create_comm_par_mxn_1d_block(name, amount, data);
}

/**
 * @brief Destroys a task.
 *
 * The user data (if any) should have been destroyed first.
 *
 * @param task the task you want to destroy
 * @see SD_task_create()
 */
void SD_task_destroy(SD_task_t task)
{
  task->destroy();
}

/**
 * @brief Returns the user data of a task
 *
 * @param task a task
 * @return the user data associated with this task (can be @c nullptr)
 * @see SD_task_set_data()
 */
void* SD_task_get_data(const_SD_task_t task)
{
  return task->get_data();
}

/**
 * @brief Sets the user data of a task
 *
 * The new data can be @c nullptr. The old data should have been freed first, if it was not @c nullptr.
 *
 * @param task a task
 * @param data the new data you want to associate with this task
 * @see SD_task_get_data()
 */
void SD_task_set_data(SD_task_t task, void* data)
{
  task->set_data(data);
}

/**
 * @brief Sets the rate of a task
 *
 * This will change the network bandwidth a task can use. This rate  cannot be dynamically changed. Once the task has
 * started, this call is ineffective. This rate depends on both the nominal bandwidth on the route onto which the task
 * is scheduled (@see SD_task_get_current_bandwidth) and the amount of data to transfer.
 *
 * To divide the nominal bandwidth by 2, the rate then has to be :
 *    rate = bandwidth/(2*amount)
 *
 * @param task a @see SD_TASK_COMM_E2E task (end-to-end communication)
 * @param rate the new rate you want to associate with this task.
 */
void SD_task_set_rate(SD_task_t task, double rate)
{
  task->set_rate(rate);
}

/**
 * @brief Returns the state of a task
 *
 * @param task a task
 * @return the current @ref e_SD_task_state_t "state" of this task:
 * #SD_NOT_SCHEDULED, #SD_SCHEDULED, #SD_RUNNABLE, #SD_RUNNING, #SD_DONE or #SD_FAILED
 * @see e_SD_task_state_t
 */
e_SD_task_state_t SD_task_get_state(const_SD_task_t task)
{
  return task->get_state();
}
/**
 * @brief Returns the name of a task
 *
 * @param task a task
 * @return the name of this task (can be @c nullptr)
 */
const char* SD_task_get_name(const_SD_task_t task)
{
  return task->get_cname();
}

/** @brief Allows to change the name of a task */
void SD_task_set_name(SD_task_t task, const char* name)
{
  task->set_name(name);
}

/** @brief Returns the dynar of the parents of a task
 *
 * @param task a task
 * @return a newly allocated dynar comprising the parents of this task
 */

xbt_dynar_t SD_task_get_parents(const_SD_task_t task)
{
  xbt_dynar_t parents = xbt_dynar_new(sizeof(SD_task_t), nullptr);

  for (auto const& it : task->get_predecessors())
    xbt_dynar_push(parents, &it);
  for (auto const& it : task->get_inputs())
    xbt_dynar_push(parents, &it);

  return parents;
}

/** @brief Returns the dynar of the parents of a task
 *
 * @param task a task
 * @return a newly allocated dynar comprising the parents of this task
 */
xbt_dynar_t SD_task_get_children(const_SD_task_t task)
{
  xbt_dynar_t children = xbt_dynar_new(sizeof(SD_task_t), nullptr);

  for (auto const& it : task->get_successors())
    xbt_dynar_push(children, &it);
  for (auto const& it : task->get_outputs())
    xbt_dynar_push(children, &it);

  return children;
}

/**
 * @brief Returns the number of workstations involved in a task
 *
 * Only call this on already scheduled tasks!
 * @param task a task
 */
int SD_task_get_workstation_count(const_SD_task_t task)
{
  return static_cast<int>(task->get_allocation_size());
}

/**
 * @brief Returns the list of workstations involved in a task
 *
 * Only call this on already scheduled tasks!
 * @param task a task
 */
sg_host_t* SD_task_get_workstation_list(const_SD_task_t task)
{
  return task->get_allocation()->data();
}

/**
 * @brief Returns the total amount of work contained in a task
 *
 * @param task a task
 * @return the total amount of work (computation or data transfer) for this task
 * @see SD_task_get_remaining_amount()
 */
double SD_task_get_amount(const_SD_task_t task)
{
  return task->get_amount();
}

/** @brief Sets the total amount of work of a task
 * For sequential typed tasks (COMP_SEQ and COMM_E2E), it also sets the appropriate values in the flops_amount and
 * bytes_amount arrays respectively. Nothing more than modifying task->amount is done for parallel  typed tasks
 * (COMP_PAR_AMDAHL and COMM_PAR_MXN_1D_BLOCK) as the distribution of the amount of work is done at scheduling time.
 *
 * @param task a task
 * @param amount the new amount of work to execute
 */
void SD_task_set_amount(SD_task_t task, double amount)
{
  task->set_amount(amount);
}

/**
 * @brief Returns the alpha parameter of a SD_TASK_COMP_PAR_AMDAHL task
 *
 * @param task a parallel task assuming Amdahl's law as speedup model
 * @return the alpha parameter (serial part of a task in percent) for this task
 */
double SD_task_get_alpha(const_SD_task_t task)
{
  return task->get_alpha();
}

/**
 * @brief Returns the remaining amount work to do till the completion of a task
 *
 * @param task a task
 * @return the remaining amount of work (computation or data transfer) of this task
 * @see SD_task_get_amount()
 */
double SD_task_get_remaining_amount(const_SD_task_t task)
{
  return task->get_remaining_amount();
}

e_SD_task_kind_t SD_task_get_kind(const_SD_task_t task)
{
  return task->get_kind();
}

/** @brief Displays debugging information about a task */
void SD_task_dump(const_SD_task_t task)
{
  task->dump();
}

/** @brief Dumps the task in dotty formalism into the FILE* passed as second argument */
void SD_task_dotty(const_SD_task_t task, void* out)
{
  auto* fout = static_cast<FILE*>(out);
  fprintf(fout, "  T%p [label=\"%.20s\"", task, task->get_cname());
  switch (task->get_kind()) {
    case SD_TASK_COMM_E2E:
    case SD_TASK_COMM_PAR_MXN_1D_BLOCK:
      fprintf(fout, ", shape=box");
      break;
    case SD_TASK_COMP_SEQ:
    case SD_TASK_COMP_PAR_AMDAHL:
      fprintf(fout, ", shape=circle");
      break;
    default:
      xbt_die("Unknown task type!");
  }
  fprintf(fout, "];\n");
  for (auto const& it : task->get_predecessors())
    fprintf(fout, " T%p -> T%p;\n", it, task);
  for (auto const& it : task->get_inputs())
    fprintf(fout, " T%p -> T%p;\n", it, task);
}

/**
 * @brief Adds a dependency between two tasks
 *
 * @a dst will depend on @a src, ie @a dst will not start before @a src is finished.
 * Their @ref e_SD_task_state_t "state" must be #SD_NOT_SCHEDULED, #SD_SCHEDULED or #SD_RUNNABLE.
 *
 * @param src the task which must be executed first
 * @param dst the task you want to make depend on @a src
 * @see SD_task_dependency_remove()
 */
void SD_task_dependency_add(SD_task_t src, SD_task_t dst)
{
  if (src == dst)
    throw std::invalid_argument(
        simgrid::xbt::string_printf("Cannot add a dependency between task '%s' and itself", SD_task_get_name(src)));

  if (src->get_state() == SD_DONE || src->get_state() == SD_FAILED)
    throw std::invalid_argument(simgrid::xbt::string_printf(
        "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULABLE, SD_SCHEDULED, SD_RUNNABLE, or SD_RUNNING",
        src->get_cname()));

  if (dst->get_state() == SD_DONE || dst->get_state() == SD_FAILED || dst->get_state() == SD_RUNNING)
    throw std::invalid_argument(simgrid::xbt::string_printf(
        "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULABLE, SD_SCHEDULED, or SD_RUNNABLE", dst->get_cname()));

  if (dst->is_child_of(src) || src->is_parent_of(dst))
    throw std::invalid_argument(simgrid::xbt::string_printf(
        "A dependency already exists between task '%s' and task '%s'", src->get_cname(), dst->get_cname()));

  XBT_DEBUG("SD_task_dependency_add: src = %s, dst = %s", src->get_cname(), dst->get_cname());

  if (src->get_kind() == SD_TASK_COMM_E2E || src->get_kind() == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
    if (dst->get_kind() == SD_TASK_COMP_SEQ || dst->get_kind() == SD_TASK_COMP_PAR_AMDAHL)
      dst->add_input(src);
    else
      dst->add_predecessor(src);
    src->add_successor(dst);
  } else {
    if (dst->get_kind() == SD_TASK_COMM_E2E || dst->get_kind() == SD_TASK_COMM_PAR_MXN_1D_BLOCK)
      src->add_output(dst);
    else
      src->add_successor(dst);
    dst->add_predecessor(src);
  }

  /* if the task was runnable, the task goes back to SD_SCHEDULED because of the new dependency*/
  if (dst->get_state() == SD_RUNNABLE) {
    XBT_DEBUG("SD_task_dependency_add: %s was runnable and becomes scheduled!", dst->get_cname());
    dst->set_state(SD_SCHEDULED);
  }
}

/**
 * @brief Indicates whether there is a dependency between two tasks.
 *
 * @param src a task
 * @param dst a task depending on @a src
 *
 * If src is nullptr, checks whether dst has any pre-dependency.
 * If dst is nullptr, checks whether src has any post-dependency.
 */
int SD_task_dependency_exists(const_SD_task_t src, SD_task_t dst)
{
  xbt_assert(src != nullptr || dst != nullptr, "Invalid parameter: both src and dst are nullptr");

  if (src)
    if (dst)
      return src->is_parent_of(dst);
    else
      return static_cast<int>(src->is_waited_by());
  else
    return static_cast<int>(dst->has_unsolved_dependencies());
}

/**
 * @brief Remove a dependency between two tasks
 *
 * @param src a task
 * @param dst a task depending on @a src
 * @see SD_task_dependency_add()
 */
void SD_task_dependency_remove(SD_task_t src, SD_task_t dst)
{
  XBT_DEBUG("SD_task_dependency_remove: src = %s, dst = %s", src->get_cname(), dst->get_cname());

  if (not src->is_parent_of(dst))
    throw std::invalid_argument(simgrid::xbt::string_printf(
        "No dependency found between task '%s' and '%s': task '%s' is not a successor of task '%s'", src->get_cname(),
        dst->get_cname(), dst->get_cname(), src->get_cname()));

  if (src->get_kind() == SD_TASK_COMM_E2E || src->get_kind() == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
    if (dst->get_kind() == SD_TASK_COMP_SEQ || dst->get_kind() == SD_TASK_COMP_PAR_AMDAHL)
      dst->rm_input(src);
    else
      dst->rm_predecessor(src);
    src->rm_successor(dst);
  } else {
    if (dst->get_kind() == SD_TASK_COMM_E2E || dst->get_kind() == SD_TASK_COMM_PAR_MXN_1D_BLOCK)
      src->rm_output(dst);
    else
      src->rm_successor(dst);
    dst->rm_predecessor(src);
  }

  /* if the task was scheduled and dependencies are satisfied, we can make it runnable */
  if (dst->has_unsolved_dependencies() == 0 && dst->get_state() == SD_SCHEDULED)
    dst->set_state(SD_RUNNABLE);
}

/**
 * @brief Adds a watch point to a task
 *
 * SD_simulate() will stop as soon as the @ref e_SD_task_state_t "state" of this task becomes the one given in argument.
 * The watch point is then automatically removed.
 *
 * @param task a task
 * @param state the @ref e_SD_task_state_t "state" you want to watch (cannot be #SD_NOT_SCHEDULED)
 * @see SD_task_unwatch()
 */
void SD_task_watch(SD_task_t task, e_SD_task_state_t state)
{
  task->watch(state);
}

/**
 * @brief Removes a watch point from a task
 *
 * @param task a task
 * @param state the @ref e_SD_task_state_t "state" you no longer want to watch
 * @see SD_task_watch()
 */
void SD_task_unwatch(SD_task_t task, e_SD_task_state_t state)
{
  task->unwatch(state);
}

/**
 * @brief Returns an approximative estimation of the execution time of a task.
 *
 * The estimation is very approximative because the value returned is the time the task would take if it was executed
 * now and if it was the only task.
 *
 * @param host_count number of hosts on which the task would be executed
 * @param host_list the hosts on which the task would be executed
 * @param flops_amount computation amount for each host(i.e., an array of host_count doubles)
 * @param bytes_amount communication amount between each pair of hosts (i.e., a matrix of host_count*host_count doubles)
 * @see SD_schedule()
 */
double SD_task_get_execution_time(const_SD_task_t /*task*/, int host_count, const sg_host_t* host_list,
                                  const double* flops_amount, const double* bytes_amount)
{
  xbt_assert(host_count > 0, "Invalid parameter");
  double max_time = 0.0;

  /* the task execution time is the maximum execution time of the parallel tasks */
  for (int i = 0; i < host_count; i++) {
    double time = 0.0;
    if (flops_amount != nullptr)
      time = flops_amount[i] / host_list[i]->get_speed();

    if (bytes_amount != nullptr)
      for (int j = 0; j < host_count; j++)
        if (bytes_amount[i * host_count + j] != 0)
          time += (sg_host_get_route_latency(host_list[i], host_list[j]) +
                   bytes_amount[i * host_count + j] / sg_host_get_route_bandwidth(host_list[i], host_list[j]));

    if (time > max_time)
      max_time = time;
  }
  return max_time;
}

/**
 * @brief Schedules a task
 *
 * The task state must be #SD_NOT_SCHEDULED.
 * Once scheduled, a task is executed as soon as possible in @see SD_simulate, i.e. when its dependencies are satisfied.
 *
 * @param task the task you want to schedule
 * @param host_count number of hosts on which the task will be executed
 * @param host_list the hosts on which the task will be executed
 * @param flops_amount computation amount for each hosts (i.e., an array of host_count doubles)
 * @param bytes_amount communication amount between each pair of hosts (i.e., a matrix of host_count*host_count doubles)
 * @param rate task execution speed rate
 * @see SD_task_unschedule()
 */
void SD_task_schedule(SD_task_t task, int host_count, const sg_host_t* host_list, const double* flops_amount,
                      const double* bytes_amount, double rate)
{
  xbt_assert(host_count > 0, "host_count must be positive");
  std::vector<sg_host_t> hosts(host_count);

  for (int i = 0; i < host_count; i++)
    hosts[i] = host_list[i];

  task->schedule(hosts, flops_amount, bytes_amount, rate);
}

/**
 * @brief Unschedules a task
 *
 * The task state must be #SD_SCHEDULED, #SD_RUNNABLE, #SD_RUNNING or #SD_FAILED.
 * If you call this function, the task state becomes #SD_NOT_SCHEDULED.
 * Call SD_task_schedule() to schedule it again.
 *
 * @param task the task you want to unschedule
 * @see SD_task_schedule()
 */
void SD_task_unschedule(SD_task_t task)
{
  task->unschedule();
}

/**
 * @brief Returns the start time of a task
 *
 * The task state must be SD_RUNNING, SD_DONE or SD_FAILED.
 *
 * @param task: a task
 * @return the start time of this task
 */
double SD_task_get_start_time(const_SD_task_t task)
{
  return task->get_start_time();
}

/**
 * @brief Returns the finish time of a task
 *
 * The task state must be SD_RUNNING, SD_DONE or SD_FAILED.
 * If the state is not completed yet, the returned value is an estimation of the task finish time. This value can
 * vary until the task is completed.
 *
 * @param task: a task
 * @return the start time of this task
 */
double SD_task_get_finish_time(const_SD_task_t task)
{
  return task->get_finish_time();
}

void SD_task_distribute_comp_amdahl(SD_task_t task, int count)
{
  task->distribute_comp_amdahl(count);
}

void SD_task_build_MxN_1D_block_matrix(SD_task_t task, int src_nb, int dst_nb)
{
  task->build_MxN_1D_block_matrix(src_nb, dst_nb);
}

/** @brief Auto-schedules a task.
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows one to specify the task costs at
 * creation, and decouple them from the scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * To be auto-schedulable, a task must be a typed computation SD_TASK_COMP_SEQ or SD_TASK_COMP_PAR_AMDAHL.
 */
void SD_task_schedulev(SD_task_t task, int count, const sg_host_t* host_list)
{
  std::vector<sg_host_t> list(count);
  for (int i = 0; i < count; i++)
    list[i] = host_list[i];
  task->schedulev(list);
}

/** @brief autoschedule a task on a list of hosts
 *
 * This function is similar to SD_task_schedulev(), but takes the list of hosts to schedule onto as separate parameters.
 * It builds a proper vector of hosts and then call SD_task_schedulev()
 */
void SD_task_schedulel(SD_task_t task, int count, ...)
{
  va_list ap;
  std::vector<sg_host_t> list(count);
  va_start(ap, count);
  for (int i = 0; i < count; i++)
    list[i] = va_arg(ap, sg_host_t);

  va_end(ap);
  task->schedulev(list);
}
