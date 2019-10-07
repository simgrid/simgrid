/* Copyright (c) 2006-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simdag_private.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/surf_interface.hpp"
#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_task, sd, "Logging specific to SimDag (task)");

/* Destroys the data memorized by SD_task_schedule. Task state must be SD_SCHEDULED or SD_RUNNABLE. */
static void __SD_task_destroy_scheduling_data(SD_task_t task)
{
  if (task->state != SD_SCHEDULED && task->state != SD_RUNNABLE)
    throw std::invalid_argument(
        simgrid::xbt::string_printf("Task '%s' must be SD_SCHEDULED or SD_RUNNABLE", SD_task_get_name(task)));

  xbt_free(task->flops_amount);
  xbt_free(task->bytes_amount);
  task->bytes_amount = nullptr;
  task->flops_amount = nullptr;
}

/**
 * @brief Creates a new task.
 *
 * @param name the name of the task (can be @c nullptr)
 * @param data the user data you want to associate with the task (can be @c nullptr)
 * @param amount amount of the task
 * @return the new task
 * @see SD_task_destroy()
 */
SD_task_t SD_task_create(const char *name, void *data, double amount)
{
  SD_task_t task = xbt_new0(s_SD_task_t, 1);
  task->kind = SD_TASK_NOT_TYPED;
  task->state= SD_NOT_SCHEDULED;
  sd_global->initial_tasks.insert(task);

  task->marked = 0;
  task->start_time = -1.0;
  task->finish_time = -1.0;
  task->surf_action = nullptr;
  task->watch_points = 0;

  task->inputs = new std::set<SD_task_t>();
  task->outputs = new std::set<SD_task_t>();
  task->predecessors = new std::set<SD_task_t>();
  task->successors = new std::set<SD_task_t>();

  task->data = data;
  task->name = xbt_strdup(name);
  task->amount = amount;
  task->allocation = new std::vector<sg_host_t>();
  task->rate = -1;
  return task;
}

static inline SD_task_t SD_task_create_sized(const char *name, void *data, double amount, int count)
{
  SD_task_t task = SD_task_create(name, data, amount);
  task->bytes_amount = xbt_new0(double, count * count);
  task->flops_amount = xbt_new0(double, count);
  return task;
}

/** @brief create a end-to-end communication task that can then be auto-scheduled
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows one to specify the task costs at
 * creation, and decouple them from the scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * A end-to-end communication must be scheduled on 2 hosts, and the amount specified at creation is sent from hosts[0]
 * to hosts[1].
 */
SD_task_t SD_task_create_comm_e2e(const char *name, void *data, double amount)
{
  SD_task_t res = SD_task_create_sized(name, data, amount, 2);
  res->bytes_amount[2] = amount;
  res->kind = SD_TASK_COMM_E2E;

  return res;
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
SD_task_t SD_task_create_comp_seq(const char *name, void *data, double flops_amount)
{
  SD_task_t res = SD_task_create_sized(name, data, flops_amount, 1);
  res->flops_amount[0] = flops_amount;
  res->kind = SD_TASK_COMP_SEQ;

  return res;
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
SD_task_t SD_task_create_comp_par_amdahl(const char *name, void *data, double flops_amount, double alpha)
{
  xbt_assert(alpha < 1. && alpha >= 0., "Invalid parameter: alpha must be in [0.;1.[");

  SD_task_t res = SD_task_create(name, data, flops_amount);
  res->alpha = alpha;
  res->kind = SD_TASK_COMP_PAR_AMDAHL;

  return res;
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
SD_task_t SD_task_create_comm_par_mxn_1d_block(const char *name, void *data, double amount)
{
  SD_task_t res = SD_task_create(name, data, amount);
  res->kind = SD_TASK_COMM_PAR_MXN_1D_BLOCK;

  return res;
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
  XBT_DEBUG("Destroying task %s...", SD_task_get_name(task));

  /* First Remove all dependencies associated with the task. */
  while (not task->predecessors->empty())
    SD_task_dependency_remove(*(task->predecessors->begin()), task);
  while (not task->inputs->empty())
    SD_task_dependency_remove(*(task->inputs->begin()), task);
  while (not task->successors->empty())
    SD_task_dependency_remove(task, *(task->successors->begin()));
  while (not task->outputs->empty())
    SD_task_dependency_remove(task, *(task->outputs->begin()));

  if (task->state == SD_SCHEDULED || task->state == SD_RUNNABLE)
    __SD_task_destroy_scheduling_data(task);

  xbt_free(task->name);

  if (task->surf_action != nullptr)
    task->surf_action->unref();

  delete task->allocation;
  xbt_free(task->bytes_amount);
  xbt_free(task->flops_amount);
  delete task->inputs;
  delete task->outputs;
  delete task->predecessors;
  delete task->successors;
  xbt_free(task);

  XBT_DEBUG("Task destroyed.");
}

/**
 * @brief Returns the user data of a task
 *
 * @param task a task
 * @return the user data associated with this task (can be @c nullptr)
 * @see SD_task_set_data()
 */
void *SD_task_get_data(SD_task_t task)
{
  return task->data;
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
void SD_task_set_data(SD_task_t task, void *data)
{
  task->data = data;
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
  xbt_assert(task->kind == SD_TASK_COMM_E2E, "The rate can be modified for end-to-end communications only.");
  if(task->state < SD_RUNNING) {
    task->rate = rate;
  } else {
    XBT_WARN("Task %p has started. Changing rate is ineffective.", task);
  }
}

/**
 * @brief Returns the state of a task
 *
 * @param task a task
 * @return the current @ref e_SD_task_state_t "state" of this task:
 * #SD_NOT_SCHEDULED, #SD_SCHEDULED, #SD_RUNNABLE, #SD_RUNNING, #SD_DONE or #SD_FAILED
 * @see e_SD_task_state_t
 */
e_SD_task_state_t SD_task_get_state(SD_task_t task)
{
  return task->state;
}

/* Changes the state of a task. Updates the sd_global->watch_point_reached flag.
 */
void SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state)
{
  std::set<SD_task_t>::iterator idx;
  XBT_DEBUG("Set state of '%s' to %d", task->name, new_state);
  if ((new_state == SD_NOT_SCHEDULED || new_state == SD_SCHEDULABLE) && task->state == SD_FAILED){
    sd_global->completed_tasks.erase(task);
    sd_global->initial_tasks.insert(task);
  }

  if (new_state == SD_SCHEDULED && task->state == SD_RUNNABLE){
    sd_global->initial_tasks.insert(task);
    sd_global->runnable_tasks.erase(task);
  }

  if (new_state == SD_RUNNABLE){
    idx = sd_global->initial_tasks.find(task);
    if (idx != sd_global->initial_tasks.end()) {
      sd_global->runnable_tasks.insert(*idx);
      sd_global->initial_tasks.erase(idx);
    }
  }

  if (new_state == SD_RUNNING)
    sd_global->runnable_tasks.erase(task);

  if (new_state == SD_DONE || new_state == SD_FAILED){
    sd_global->completed_tasks.insert(task);
    task->start_time = task->surf_action->get_start_time();
    if (new_state == SD_DONE){
      task->finish_time = task->surf_action->get_finish_time();
#if SIMGRID_HAVE_JEDULE
      jedule_log_sd_event(task);
#endif
    } else
      task->finish_time = surf_get_clock();
    task->surf_action->unref();
    task->surf_action = nullptr;
    task->allocation->clear();
  }

  task->state = new_state;

  if (task->watch_points & new_state) {
    XBT_VERB("Watch point reached with task '%s'!", task->name);
    sd_global->watch_point_reached = true;
    SD_task_unwatch(task, new_state);   /* remove the watch point */
  }
}

/**
 * @brief Returns the name of a task
 *
 * @param task a task
 * @return the name of this task (can be @c nullptr)
 */
const char *SD_task_get_name(SD_task_t task)
{
  return task->name;
}

/** @brief Allows to change the name of a task */
void SD_task_set_name(SD_task_t task, const char *name)
{
  xbt_free(task->name);
  task->name = xbt_strdup(name);
}

/** @brief Returns the dynar of the parents of a task
 *
 * @param task a task
 * @return a newly allocated dynar comprising the parents of this task
 */

xbt_dynar_t SD_task_get_parents(SD_task_t task)
{
  xbt_dynar_t parents = xbt_dynar_new(sizeof(SD_task_t), nullptr);

  for (auto const& it : *task->predecessors)
    xbt_dynar_push(parents, &it);
  for (auto const& it : *task->inputs)
    xbt_dynar_push(parents, &it);

  return parents;
}

/** @brief Returns the dynar of the parents of a task
 *
 * @param task a task
 * @return a newly allocated dynar comprising the parents of this task
 */
xbt_dynar_t SD_task_get_children(SD_task_t task)
{
  xbt_dynar_t children = xbt_dynar_new(sizeof(SD_task_t), nullptr);

  for (auto const& it : *task->successors)
    xbt_dynar_push(children, &it);
  for (auto const& it : *task->outputs)
    xbt_dynar_push(children, &it);

  return children;
}

/**
 * @brief Returns the number of workstations involved in a task
 *
 * Only call this on already scheduled tasks!
 * @param task a task
 */
int SD_task_get_workstation_count(SD_task_t task)
{
  return task->allocation->size();
}

/**
 * @brief Returns the list of workstations involved in a task
 *
 * Only call this on already scheduled tasks!
 * @param task a task
 */
sg_host_t *SD_task_get_workstation_list(SD_task_t task)
{
  return task->allocation->data();
}

/**
 * @brief Returns the total amount of work contained in a task
 *
 * @param task a task
 * @return the total amount of work (computation or data transfer) for this task
 * @see SD_task_get_remaining_amount()
 */
double SD_task_get_amount(SD_task_t task)
{
  return task->amount;
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
  task->amount = amount;
  if (task->kind == SD_TASK_COMP_SEQ)
    task->flops_amount[0] = amount;
  if (task->kind == SD_TASK_COMM_E2E)
    task->bytes_amount[2] = amount;
}

/**
 * @brief Returns the alpha parameter of a SD_TASK_COMP_PAR_AMDAHL task
 *
 * @param task a parallel task assuming Amdahl's law as speedup model
 * @return the alpha parameter (serial part of a task in percent) for this task
 */
double SD_task_get_alpha(SD_task_t task)
{
  xbt_assert(SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL, "Alpha parameter is not defined for this kind of task");
  return task->alpha;
}

/**
 * @brief Returns the remaining amount work to do till the completion of a task
 *
 * @param task a task
 * @return the remaining amount of work (computation or data transfer) of this task
 * @see SD_task_get_amount()
 */
double SD_task_get_remaining_amount(SD_task_t task)
{
  if (task->surf_action)
    return task->surf_action->get_remains();
  else
    return (task->state == SD_DONE) ? 0 : task->amount;
}

e_SD_task_kind_t SD_task_get_kind(SD_task_t task)
{
  return task->kind;
}

/** @brief Displays debugging information about a task */
void SD_task_dump(SD_task_t task)
{
  XBT_INFO("Displaying task %s", SD_task_get_name(task));
  if (task->state == SD_RUNNABLE)
    XBT_INFO("  - state: runnable");
  else if (task->state < SD_RUNNABLE)
    XBT_INFO("  - state: %s not runnable", __get_state_name(task->state));
  else
    XBT_INFO("  - state: not runnable %s", __get_state_name(task->state));

  if (task->kind != 0) {
    switch (task->kind) {
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
      XBT_INFO("  - (unknown kind %d)", task->kind);
    }
  }

  XBT_INFO("  - amount: %.0f", SD_task_get_amount(task));
  if (task->kind == SD_TASK_COMP_PAR_AMDAHL)
    XBT_INFO("  - alpha: %.2f", task->alpha);
  XBT_INFO("  - Dependencies to satisfy: %zu", task->inputs->size()+ task->predecessors->size());
  if ((task->inputs->size()+ task->predecessors->size()) > 0) {
    XBT_INFO("  - pre-dependencies:");
    for (auto const& it : *task->predecessors)
      XBT_INFO("    %s", it->name);

    for (auto const& it : *task->inputs)
      XBT_INFO("    %s", it->name);
  }
  if ((task->outputs->size() + task->successors->size()) > 0) {
    XBT_INFO("  - post-dependencies:");

    for (auto const& it : *task->successors)
      XBT_INFO("    %s", it->name);
    for (auto const& it : *task->outputs)
      XBT_INFO("    %s", it->name);
  }
}

/** @brief Dumps the task in dotty formalism into the FILE* passed as second argument */
void SD_task_dotty(SD_task_t task, void *out)
{
  FILE *fout = static_cast<FILE*>(out);
  fprintf(fout, "  T%p [label=\"%.20s\"", task, task->name);
  switch (task->kind) {
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
  for (auto const& it : *task->predecessors)
    fprintf(fout, " T%p -> T%p;\n", it, task);
  for (auto const& it : *task->inputs)
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

  if (src->state == SD_DONE || src->state == SD_FAILED)
    throw std::invalid_argument(simgrid::xbt::string_printf(
        "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULABLE, SD_SCHEDULED, SD_RUNNABLE, or SD_RUNNING", src->name));

  if (dst->state == SD_DONE || dst->state == SD_FAILED || dst->state == SD_RUNNING)
    throw std::invalid_argument(simgrid::xbt::string_printf(
        "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULABLE, SD_SCHEDULED, or SD_RUNNABLE", dst->name));

  if (dst->inputs->find(src) != dst->inputs->end() || src->outputs->find(dst) != src->outputs->end() ||
      src->successors->find(dst) != src->successors->end() || dst->predecessors->find(src) != dst->predecessors->end())
    throw std::invalid_argument(simgrid::xbt::string_printf(
        "A dependency already exists between task '%s' and task '%s'", src->name, dst->name));

  XBT_DEBUG("SD_task_dependency_add: src = %s, dst = %s", src->name, dst->name);

  if (src->kind == SD_TASK_COMM_E2E || src->kind == SD_TASK_COMM_PAR_MXN_1D_BLOCK){
    if (dst->kind == SD_TASK_COMP_SEQ || dst->kind == SD_TASK_COMP_PAR_AMDAHL)
        dst->inputs->insert(src);
    else
      dst->predecessors->insert(src);
    src->successors->insert(dst);
  } else {
    if (dst->kind == SD_TASK_COMM_E2E|| dst->kind == SD_TASK_COMM_PAR_MXN_1D_BLOCK)
      src->outputs->insert(dst);
    else
      src->successors->insert(dst);
    dst->predecessors->insert(src);
  }

  /* if the task was runnable, the task goes back to SD_SCHEDULED because of the new dependency*/
  if (dst->state == SD_RUNNABLE) {
    XBT_DEBUG("SD_task_dependency_add: %s was runnable and becomes scheduled!", dst->name);
    SD_task_set_state(dst, SD_SCHEDULED);
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
int SD_task_dependency_exists(SD_task_t src, SD_task_t dst)
{
  xbt_assert(src != nullptr || dst != nullptr, "Invalid parameter: both src and dst are nullptr");

  if (src) {
    if (dst) {
      return (src->successors->find(dst) != src->successors->end() || src->outputs->find(dst) != src->outputs->end());
    } else {
      return src->successors->size() + src->outputs->size();
    }
  } else {
    return dst->predecessors->size() + dst->inputs->size();
  }
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
  XBT_DEBUG("SD_task_dependency_remove: src = %s, dst = %s", SD_task_get_name(src), SD_task_get_name(dst));

  if (src->successors->find(dst) == src->successors->end() && src->outputs->find(dst) == src->outputs->end())
    throw std::invalid_argument(simgrid::xbt::string_printf(
        "No dependency found between task '%s' and '%s': task '%s' is not a successor of task '%s'", src->name,
        dst->name, dst->name, src->name));

  if (src->kind == SD_TASK_COMM_E2E || src->kind == SD_TASK_COMM_PAR_MXN_1D_BLOCK){
    if (dst->kind == SD_TASK_COMP_SEQ || dst->kind == SD_TASK_COMP_PAR_AMDAHL)
      dst->inputs->erase(src);
    else
      dst->predecessors->erase(src);
    src->successors->erase(dst);
  } else {
    if (dst->kind == SD_TASK_COMM_E2E|| dst->kind == SD_TASK_COMM_PAR_MXN_1D_BLOCK)
      src->outputs->erase(dst);
    else
      src->successors->erase(dst);
    dst->predecessors->erase(src);
  }

  /* if the task was scheduled and dependencies are satisfied, we can make it runnable */
  if (dst->predecessors->empty() && dst->inputs->empty() && dst->state == SD_SCHEDULED)
    SD_task_set_state(dst, SD_RUNNABLE);
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
  if (state & SD_NOT_SCHEDULED)
    throw std::invalid_argument("Cannot add a watch point for state SD_NOT_SCHEDULED");

  task->watch_points = task->watch_points | state;
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
  xbt_assert(state != SD_NOT_SCHEDULED, "SimDag error: Cannot have a watch point for state SD_NOT_SCHEDULED");
  task->watch_points = task->watch_points & ~state;
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
double SD_task_get_execution_time(SD_task_t /*task*/, int host_count, const sg_host_t* host_list,
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
          time += (sg_host_route_latency(host_list[i], host_list[j]) +
                   bytes_amount[i * host_count + j] / sg_host_route_bandwidth(host_list[i], host_list[j]));

    if (time > max_time)
      max_time = time;
  }
  return max_time;
}

static inline void SD_task_do_schedule(SD_task_t task)
{
  if (SD_task_get_state(task) > SD_SCHEDULABLE)
    throw std::invalid_argument(
        simgrid::xbt::string_printf("Task '%s' has already been scheduled", SD_task_get_name(task)));

  if (task->predecessors->empty() && task->inputs->empty())
    SD_task_set_state(task, SD_RUNNABLE);
  else
    SD_task_set_state(task, SD_SCHEDULED);
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
void SD_task_schedule(SD_task_t task, int host_count, const sg_host_t * host_list,
                      const double *flops_amount, const double *bytes_amount, double rate)
{
  xbt_assert(host_count > 0, "host_count must be positive");

  task->rate = rate;

  if (flops_amount) {
    task->flops_amount = static_cast<double*>(xbt_realloc(task->flops_amount, sizeof(double) * host_count));
    memcpy(task->flops_amount, flops_amount, sizeof(double) * host_count);
  } else {
    xbt_free(task->flops_amount);
    task->flops_amount = nullptr;
  }

  int communication_nb = host_count * host_count;
  if (bytes_amount) {
    task->bytes_amount = static_cast<double*>(xbt_realloc(task->bytes_amount, sizeof(double) * communication_nb));
    memcpy(task->bytes_amount, bytes_amount, sizeof(double) * communication_nb);
  } else {
    xbt_free(task->bytes_amount);
    task->bytes_amount = nullptr;
  }

  for(int i =0; i<host_count; i++)
    task->allocation->push_back(host_list[i]);

  SD_task_do_schedule(task);
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
  if (task->state == SD_NOT_SCHEDULED || task->state == SD_SCHEDULABLE)
    throw std::invalid_argument(simgrid::xbt::string_printf(
        "Task %s: the state must be SD_SCHEDULED, SD_RUNNABLE, SD_RUNNING or SD_FAILED", task->name));

  if ((task->state == SD_SCHEDULED || task->state == SD_RUNNABLE) /* if the task is scheduled or runnable */
      && ((task->kind == SD_TASK_COMP_PAR_AMDAHL) || (task->kind == SD_TASK_COMM_PAR_MXN_1D_BLOCK))) {
          /* Don't free scheduling data for typed tasks */
    __SD_task_destroy_scheduling_data(task);
    task->allocation->clear();
  }

  if (SD_task_get_state(task) == SD_RUNNING)
    /* the task should become SD_FAILED */
    task->surf_action->cancel();
  else {
    if (task->predecessors->empty() && task->inputs->empty())
      SD_task_set_state(task, SD_SCHEDULABLE);
    else
      SD_task_set_state(task, SD_NOT_SCHEDULED);
  }
  task->start_time = -1.0;
}

/* Runs a task. */
void SD_task_run(SD_task_t task)
{
  xbt_assert(task->state == SD_RUNNABLE, "Task '%s' is not runnable! Task state: %d", task->name, (int) task->state);
  xbt_assert(task->allocation != nullptr, "Task '%s': host_list is nullptr!", task->name);

  XBT_VERB("Executing task '%s'", task->name);

  /* Beware! The scheduling data are now used by the surf action directly! no copy was done */
  task->surf_action =
      surf_host_model->execute_parallel(*task->allocation, task->flops_amount, task->bytes_amount, task->rate);

  task->surf_action->set_data(task);

  XBT_DEBUG("surf_action = %p", task->surf_action);

  SD_task_set_state(task, SD_RUNNING);
  sd_global->return_set.insert(task);
}

/**
 * @brief Returns the start time of a task
 *
 * The task state must be SD_RUNNING, SD_DONE or SD_FAILED.
 *
 * @param task: a task
 * @return the start time of this task
 */
double SD_task_get_start_time(SD_task_t task)
{
  if (task->surf_action)
    return task->surf_action->get_start_time();
  else
    return task->start_time;
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
double SD_task_get_finish_time(SD_task_t task)
{
  if (task->surf_action)        /* should never happen as actions are destroyed right after their completion */
    return task->surf_action->get_finish_time();
  else
    return task->finish_time;
}

void SD_task_distribute_comp_amdahl(SD_task_t task, int count)
{
  xbt_assert(task->kind == SD_TASK_COMP_PAR_AMDAHL, "Task %s is not a SD_TASK_COMP_PAR_AMDAHL typed task."
              "Cannot use this function.", task->name);
  task->flops_amount = xbt_new0(double, count);
  task->bytes_amount = xbt_new0(double, count * count);

  for (int i=0; i<count; i++){
    task->flops_amount[i] = (task->alpha + (1 - task->alpha)/count) * task->amount;
  }
}

void SD_task_build_MxN_1D_block_matrix(SD_task_t task, int src_nb, int dst_nb){
  xbt_assert(task->kind == SD_TASK_COMM_PAR_MXN_1D_BLOCK, "Task %s is not a SD_TASK_COMM_PAR_MXN_1D_BLOCK typed task."
              "Cannot use this function.", task->name);
  xbt_free(task->bytes_amount);
  task->bytes_amount = xbt_new0(double,task->allocation->size() * task->allocation->size());

  for (int i=0; i<src_nb; i++) {
    double src_start = i*task->amount/src_nb;
    double src_end = src_start + task->amount/src_nb;
    for (int j=0; j<dst_nb; j++) {
      double dst_start = j*task->amount/dst_nb;
      double dst_end = dst_start + task->amount/dst_nb;
      XBT_VERB("(%d->%d): (%.2f, %.2f)-> (%.2f, %.2f)", i, j, src_start, src_end, dst_start, dst_end);
      task->bytes_amount[i*(src_nb+dst_nb)+src_nb+j]=0.0;
      if ((src_end > dst_start) && (dst_end > src_start)) { /* There is something to send */
        task->bytes_amount[i * (src_nb + dst_nb) + src_nb + j] =
            std::min(src_end, dst_end) - std::max(src_start, dst_start);
        XBT_VERB("==> %.2f", task->bytes_amount[i*(src_nb+dst_nb)+src_nb+j]);
      }
    }
  }
}

/** @brief Auto-schedules a task.
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows one to specify the task costs at
 * creation, and decouple them from the scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * To be auto-schedulable, a task must be a typed computation SD_TASK_COMP_SEQ or SD_TASK_COMP_PAR_AMDAHL.
 */
void SD_task_schedulev(SD_task_t task, int count, const sg_host_t * list)
{
  xbt_assert(task->kind == SD_TASK_COMP_SEQ || task->kind == SD_TASK_COMP_PAR_AMDAHL,
      "Task %s is not typed. Cannot automatically schedule it.", SD_task_get_name(task));

  for(int i =0; i<count; i++)
    task->allocation->push_back(list[i]);

  XBT_VERB("Schedule computation task %s on %zu host(s)", task->name, task->allocation->size());

  if (task->kind == SD_TASK_COMP_SEQ) {
    if (not task->flops_amount) { /*This task has failed and is rescheduled. Reset the flops_amount*/
      task->flops_amount = xbt_new0(double, 1);
      task->flops_amount[0] = task->amount;
    }
    XBT_VERB("It costs %.f flops", task->flops_amount[0]);
  }

  if (task->kind == SD_TASK_COMP_PAR_AMDAHL) {
    SD_task_distribute_comp_amdahl(task, count);
    XBT_VERB("%.f flops will be distributed following Amdahl's Law", task->flops_amount[0]);
  }

  SD_task_do_schedule(task);

  /* Iterate over all inputs and outputs to say where I am located (and start them if runnable) */
  for (auto const& input : *task->inputs) {
    int src_nb = input->allocation->size();
    int dst_nb = count;
    if (input->allocation->empty())
      XBT_VERB("Sender side of '%s' not scheduled. Set receiver side to '%s''s allocation", input->name, task->name);

    for (int i=0; i<count;i++)
      input->allocation->push_back(task->allocation->at(i));

    if (input->allocation->size () > task->allocation->size()) {
      if (task->kind == SD_TASK_COMP_PAR_AMDAHL)
        SD_task_build_MxN_1D_block_matrix(input, src_nb, dst_nb);

      SD_task_do_schedule(input);
      XBT_VERB ("Auto-Schedule Communication task '%s'. Send %.f bytes from %d hosts to %d hosts.",
          input->name,input->amount, src_nb, dst_nb);
    }
  }

  for (auto const& output : *task->outputs) {
    int src_nb = count;
    int dst_nb = output->allocation->size();
    if (output->allocation->empty())
      XBT_VERB("Receiver side of '%s' not scheduled. Set sender side to '%s''s allocation", output->name, task->name);

    for (int i=0; i<count;i++)
      output->allocation->insert(output->allocation->begin()+i, task->allocation->at(i));

    if (output->allocation->size () > task->allocation->size()) {
      if (task->kind == SD_TASK_COMP_PAR_AMDAHL)
        SD_task_build_MxN_1D_block_matrix(output, src_nb, dst_nb);

      SD_task_do_schedule(output);
      XBT_VERB ("Auto-Schedule Communication task %s. Send %.f bytes from %d hosts to %d hosts.",
                output->name, output->amount, src_nb, dst_nb);
    }
  }
}

/** @brief autoschedule a task on a list of hosts
 *
 * This function is similar to SD_task_schedulev(), but takes the list of hosts to schedule onto as separate parameters.
 * It builds a proper vector of hosts and then call SD_task_schedulev()
 */
void SD_task_schedulel(SD_task_t task, int count, ...)
{
  va_list ap;
  sg_host_t* list = new sg_host_t[count];
  va_start(ap, count);
  for (int i=0; i<count; i++)
    list[i] = va_arg(ap, sg_host_t);

  va_end(ap);
  SD_task_schedulev(task, count, list);
  delete[] list;
}
