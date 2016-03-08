/* Copyright (c) 2006-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/HostImpl.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/simdag/simdag_private.h"
#include "simgrid/simdag.h"
#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_task, sd, "Logging specific to SimDag (task)");

/* Destroys a dependency between two tasks. */
static void __SD_task_dependency_destroy(void *dependency)
{
  xbt_free(((SD_dependency_t)dependency)->name);
  xbt_free(dependency);
}

/* Remove all dependencies associated with a task. This function is called when the task is destroyed. */
static void __SD_task_remove_dependencies(SD_task_t task)
{
  /* we must destroy the dependencies carefuly (with SD_dependency_remove) because each one is stored twice */
  SD_dependency_t dependency;
  while (!xbt_dynar_is_empty(task->tasks_before)) {
    xbt_dynar_get_cpy(task->tasks_before, 0, &dependency);
    SD_task_dependency_remove(dependency->src, dependency->dst);
  }

  while (!xbt_dynar_is_empty(task->tasks_after)) {
    xbt_dynar_get_cpy(task->tasks_after, 0, &dependency);
    SD_task_dependency_remove(dependency->src, dependency->dst);
  }
}

/* Destroys the data memorized by SD_task_schedule. Task state must be SD_SCHEDULED or SD_RUNNABLE. */
static void __SD_task_destroy_scheduling_data(SD_task_t task)
{
  if (task->state != SD_SCHEDULED && task->state != SD_RUNNABLE)
    THROWF(arg_error, 0, "Task '%s' must be SD_SCHEDULED or SD_RUNNABLE", SD_task_get_name(task));

  xbt_free(task->flops_amount);
  xbt_free(task->bytes_amount);
  task->flops_amount = task->bytes_amount = NULL;
}

void* SD_task_new_f(void)
{
  SD_task_t task = xbt_new0(s_SD_task_t, 1);
  task->tasks_before = xbt_dynar_new(sizeof(SD_dependency_t), NULL);
  task->tasks_after = xbt_dynar_new(sizeof(SD_dependency_t), NULL);

  return task;
}

void SD_task_recycle_f(void *t)
{
  SD_task_t task = (SD_task_t) t;

  /* Reset the content */
  task->kind = SD_TASK_NOT_TYPED;
  task->state= SD_NOT_SCHEDULED;
  xbt_dynar_push(sd_global->initial_task_set,&task);

  task->marked = 0;

  task->start_time = -1.0;
  task->finish_time = -1.0;
  task->surf_action = NULL;
  task->watch_points = 0;

  /* dependencies */
  xbt_dynar_reset(task->tasks_before);
  xbt_dynar_reset(task->tasks_after);
  task->unsatisfied_dependencies = 0;
  task->is_not_ready = 0;

  /* scheduling parameters */
  task->host_count = 0;
  task->host_list = NULL;
  task->flops_amount = NULL;
  task->bytes_amount = NULL;
  task->rate = -1;
}

void SD_task_free_f(void *t)
{
  SD_task_t task = (SD_task_t)t;

  xbt_dynar_free(&task->tasks_before);
  xbt_dynar_free(&task->tasks_after);
  xbt_free(task);
}

/**
 * \brief Creates a new task.
 *
 * \param name the name of the task (can be \c NULL)
 * \param data the user data you want to associate with the task (can be \c NULL)
 * \param amount amount of the task
 * \return the new task
 * \see SD_task_destroy()
 */
SD_task_t SD_task_create(const char *name, void *data, double amount)
{
  SD_task_t task = (SD_task_t)xbt_mallocator_get(sd_global->task_mallocator);

  /* general information */
  task->data = data;            /* user data */
  task->name = xbt_strdup(name);
  task->amount = amount;
  task->remains = amount;

  return task;
}

static inline SD_task_t SD_task_create_sized(const char *name, void *data, double amount, int ws_count)
{
  SD_task_t task = SD_task_create(name, data, amount);
  task->bytes_amount = xbt_new0(double, ws_count * ws_count);
  task->flops_amount = xbt_new0(double, ws_count);
  task->host_count = ws_count;
  task->host_list = xbt_new0(sg_host_t, ws_count);
  return task;
}

/** @brief create a end-to-end communication task that can then be auto-scheduled
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows to specify the task costs at
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
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows to specify the task costs at
 * creation, and decouple them from the scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * A sequential computation must be scheduled on 1 host, and the amount specified at creation to be run on hosts[0].
 *
 * \param name the name of the task (can be \c NULL)
 * \param data the user data you want to associate with the task (can be \c NULL)
 * \param flops_amount amount of compute work to be done by the task
 * \return the new SD_TASK_COMP_SEQ typed task
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
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows to specify the task costs at
 * creation, and decouple them from the scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * A parallel computation can be scheduled on any number of host.
 * The underlying speedup model is Amdahl's law.
 * To be auto-scheduled, \see SD_task_distribute_comp_amdahl has to be called first.
 * \param name the name of the task (can be \c NULL)
 * \param data the user data you want to associate with the task (can be \c NULL)
 * \param flops_amount amount of compute work to be done by the task
 * \param alpha purely serial fraction of the work to be done (in [0.;1.[)
 * \return the new task
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
 * This allows to specify the task costs at creation, and decouple them from the scheduling process where you just
 * specify which resource should communicate.
 *
 * A data redistribution can be scheduled on any number of host.
 * The assumed distribution is a 1D block distribution. Each host owns the same share of the \see amount.
 * To be auto-scheduled, \see SD_task_distribute_comm_mxn_1d_block has to be  called first.
 * \param name the name of the task (can be \c NULL)
 * \param data the user data you want to associate with the task (can be \c NULL)
 * \param amount amount of data to redistribute by the task
 * \return the new task
 */
SD_task_t SD_task_create_comm_par_mxn_1d_block(const char *name, void *data, double amount)
{
  SD_task_t res = SD_task_create(name, data, amount);
  res->host_list=NULL;
  res->kind = SD_TASK_COMM_PAR_MXN_1D_BLOCK;

  return res;
}

/**
 * \brief Destroys a task.
 *
 * The user data (if any) should have been destroyed first.
 *
 * \param task the task you want to destroy
 * \see SD_task_create()
 */
void SD_task_destroy(SD_task_t task)
{
  XBT_DEBUG("Destroying task %s...", SD_task_get_name(task));

  __SD_task_remove_dependencies(task);

  if (task->state == SD_SCHEDULED || task->state == SD_RUNNABLE)
    __SD_task_destroy_scheduling_data(task);

  int idx = xbt_dynar_search_or_negative(sd_global->return_set, &task);
  if (idx >=0) {
    xbt_dynar_remove_at(sd_global->return_set, idx, NULL);
  }

  xbt_free(task->name);

  if (task->surf_action != NULL)
    task->surf_action->unref();

  xbt_free(task->host_list);
  xbt_free(task->bytes_amount);
  xbt_free(task->flops_amount);

  xbt_mallocator_release(sd_global->task_mallocator,task);

  XBT_DEBUG("Task destroyed.");
}

/**
 * \brief Returns the user data of a task
 *
 * \param task a task
 * \return the user data associated with this task (can be \c NULL)
 * \see SD_task_set_data()
 */
void *SD_task_get_data(SD_task_t task)
{
  return task->data;
}

/**
 * \brief Sets the user data of a task
 *
 * The new data can be \c NULL. The old data should have been freed first
 * if it was not \c NULL.
 *
 * \param task a task
 * \param data the new data you want to associate with this task
 * \see SD_task_get_data()
 */
void SD_task_set_data(SD_task_t task, void *data)
{
  task->data = data;
}

/**
 * \brief Sets the rate of a task
 *
 * This will change the network bandwidth a task can use. This rate  cannot be dynamically changed. Once the task has
 * started, this call is ineffective. This rate depends on both the nominal bandwidth on the route onto which the task
 * is scheduled (\see SD_task_get_current_bandwidth) and the amount of data to transfer.
 *
 * To divide the nominal bandwidth by 2, the rate then has to be :
 *    rate = bandwidth/(2*amount)
 *
 * \param task a \see SD_TASK_COMM_E2E task (end-to-end communication)
 * \param rate the new rate you want to associate with this task.
 */
void SD_task_set_rate(SD_task_t task, double rate)
{
  xbt_assert(task->kind == SD_TASK_COMM_E2E, "The rate can be modified for end-to-end communications only.");
  if(task->start_time<0) {
    task->rate = rate;
  } else {
    XBT_WARN("Task %p has started. Changing rate is ineffective.", task);
  }
}

/**
 * \brief Returns the state of a task
 *
 * \param task a task
 * \return the current \ref e_SD_task_state_t "state" of this task:
 * #SD_NOT_SCHEDULED, #SD_SCHEDULED, #SD_RUNNABLE, #SD_RUNNING, #SD_DONE or #SD_FAILED
 * \see e_SD_task_state_t
 */
e_SD_task_state_t SD_task_get_state(SD_task_t task)
{
  return task->state;
}

/* Changes the state of a task. Updates the sd_global->watch_point_reached flag.
 */
void SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state)
{
  int idx;
  switch (new_state) {
  case SD_NOT_SCHEDULED:
  case SD_SCHEDULABLE:
    if (SD_task_get_state(task) == SD_FAILED){
        xbt_dynar_remove_at(sd_global->completed_task_set,
            xbt_dynar_search(sd_global->completed_task_set, &task), NULL);
        xbt_dynar_push(sd_global->initial_task_set,&task);
    }
    break;
  case SD_SCHEDULED:
    if (SD_task_get_state(task) == SD_RUNNABLE){
      xbt_dynar_remove_at(sd_global->executable_task_set,
          xbt_dynar_search(sd_global->executable_task_set, &task), NULL);
      xbt_dynar_push(sd_global->initial_task_set,&task);
    }
    break;
  case SD_RUNNABLE:
    idx = xbt_dynar_search_or_negative(sd_global->initial_task_set, &task);
    if (idx >= 0) {
      xbt_dynar_remove_at(sd_global->initial_task_set, idx, NULL);
      xbt_dynar_push(sd_global->executable_task_set,&task);
    }
    break;
  case SD_RUNNING:
      xbt_dynar_remove_at(sd_global->executable_task_set,
         xbt_dynar_search(sd_global->executable_task_set, &task), NULL);
    break;
  case SD_DONE:
    xbt_dynar_push(sd_global->completed_task_set,&task);
    task->finish_time = task->surf_action->getFinishTime();
    task->remains = 0;
#if HAVE_JEDULE
    jedule_log_sd_event(task);
#endif
    break;
  case SD_FAILED:
    xbt_dynar_push(sd_global->completed_task_set,&task);
    break;
  default:
    xbt_die( "Invalid state");
  }

  task->state = new_state;

  if (task->watch_points & new_state) {
    XBT_VERB("Watch point reached with task '%s'!", SD_task_get_name(task));
    sd_global->watch_point_reached = 1;
    SD_task_unwatch(task, new_state);   /* remove the watch point */
  }
}

/**
 * \brief Returns the name of a task
 *
 * \param task a task
 * \return the name of this task (can be \c NULL)
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
 * \param task a task
 * \return a newly allocated dynar comprising the parents of this task
 */

xbt_dynar_t SD_task_get_parents(SD_task_t task)
{
  unsigned int i;
  SD_dependency_t dep;

  xbt_dynar_t parents = xbt_dynar_new(sizeof(SD_task_t), NULL);
  xbt_dynar_foreach(task->tasks_before, i, dep) {
    xbt_dynar_push(parents, &(dep->src));
  }
  return parents;
}

/** @brief Returns the dynar of the parents of a task
 *
 * \param task a task
 * \return a newly allocated dynar comprising the parents of this task
 */
xbt_dynar_t SD_task_get_children(SD_task_t task)
{
  unsigned int i;
  SD_dependency_t dep;

  xbt_dynar_t children = xbt_dynar_new(sizeof(SD_task_t), NULL);
  xbt_dynar_foreach(task->tasks_after, i, dep) {
    xbt_dynar_push(children, &(dep->dst));
  }
  return children;
}

/**
 * \brief Returns the amount of workstations involved in a task
 *
 * Only call this on already scheduled tasks!
 * \param task a task
 */
int SD_task_get_workstation_count(SD_task_t task)
{
  return task->host_count;
}

/**
 * \brief Returns the list of workstations involved in a task
 *
 * Only call this on already scheduled tasks!
 * \param task a task
 */
sg_host_t *SD_task_get_workstation_list(SD_task_t task)
{
  return task->host_list;
}

/**
 * \brief Returns the total amount of work contained in a task
 *
 * \param task a task
 * \return the total amount of work (computation or data transfer) for this task
 * \see SD_task_get_remaining_amount()
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
 * \param task a task
 * \param amount the new amount of work to execute
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
 * \brief Returns the alpha parameter of a SD_TASK_COMP_PAR_AMDAHL task
 *
 * \param task a parallel task assuming Amdahl's law as speedup model
 * \return the alpha parameter (serial part of a task in percent) for this task
 */
double SD_task_get_alpha(SD_task_t task)
{
  xbt_assert(SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL, "Alpha parameter is not defined for this kind of task");
  return task->alpha;
}


/**
 * \brief Returns the remaining amount work to do till the completion of a task
 *
 * \param task a task
 * \return the remaining amount of work (computation or data transfer) of this task
 * \see SD_task_get_amount()
 */
double SD_task_get_remaining_amount(SD_task_t task)
{
  if (task->surf_action)
    return task->surf_action->getRemains();
  else
    return task->remains;
}

e_SD_task_kind_t SD_task_get_kind(SD_task_t task)
{
  return task->kind;
}

/** @brief Displays debugging informations about a task */
void SD_task_dump(SD_task_t task)
{
  unsigned int counter;
  SD_dependency_t dependency;

  XBT_INFO("Displaying task %s", SD_task_get_name(task));
  char *statename = bprintf("%s%s%s%s%s%s%s",
                      (task->state == SD_NOT_SCHEDULED ? " not scheduled" : ""),
                      (task->state == SD_SCHEDULABLE ? " schedulable" : ""),
                      (task->state == SD_SCHEDULED ? " scheduled" : ""),
                      (task->state == SD_RUNNABLE ? " runnable" : " not runnable"),
                      (task->state == SD_RUNNING ? " running" : ""),
                      (task->state == SD_DONE ? " done" : ""),
                      (task->state == SD_FAILED ? " failed" : ""));
  XBT_INFO("  - state:%s", statename);
  free(statename);

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

  if (task->category)
    XBT_INFO("  - tracing category: %s", task->category);

  XBT_INFO("  - amount: %.0f", SD_task_get_amount(task));
  if (task->kind == SD_TASK_COMP_PAR_AMDAHL)
    XBT_INFO("  - alpha: %.2f", task->alpha);
  XBT_INFO("  - Dependencies to satisfy: %d", task->unsatisfied_dependencies);
  if (!xbt_dynar_is_empty(task->tasks_before)) {
    XBT_INFO("  - pre-dependencies:");
    xbt_dynar_foreach(task->tasks_before, counter, dependency) {
      XBT_INFO("    %s", SD_task_get_name(dependency->src));
    }
  }
  if (!xbt_dynar_is_empty(task->tasks_after)) {
    XBT_INFO("  - post-dependencies:");
    xbt_dynar_foreach(task->tasks_after, counter, dependency) {
      XBT_INFO("    %s", SD_task_get_name(dependency->dst));
    }
  }
}

/** @brief Dumps the task in dotty formalism into the FILE* passed as second argument */
void SD_task_dotty(SD_task_t task, void *out)
{
  unsigned int counter;
  SD_dependency_t dependency;
  FILE *fout = (FILE*)out;
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
  xbt_dynar_foreach(task->tasks_before, counter, dependency) {
    fprintf(fout, " T%p -> T%p;\n", dependency->src, dependency->dst);
  }
}

/**
 * \brief Adds a dependency between two tasks
 *
 * \a dst will depend on \a src, ie \a dst will not start before \a src is finished.
 * Their \ref e_SD_task_state_t "state" must be #SD_NOT_SCHEDULED, #SD_SCHEDULED or #SD_RUNNABLE.
 *
 * \param name the name of the new dependency (can be \c NULL)
 * \param data the user data you want to associate with this dependency (can be \c NULL)
 * \param src the task which must be executed first
 * \param dst the task you want to make depend on \a src
 * \see SD_task_dependency_remove()
 */
void SD_task_dependency_add(const char *name, void *data, SD_task_t src, SD_task_t dst)
{
  int found = 0;
  SD_dependency_t dependency;

  unsigned long length = xbt_dynar_length(src->tasks_after);

  if (src == dst)
    THROWF(arg_error, 0, "Cannot add a dependency between task '%s' and itself", SD_task_get_name(src));

  e_SD_task_state_t state = SD_task_get_state(src);
  if (state != SD_NOT_SCHEDULED && state != SD_SCHEDULABLE && state != SD_RUNNING && state != SD_SCHEDULED &&
       state != SD_RUNNABLE)
    THROWF(arg_error, 0, "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULABLE, SD_SCHEDULED, SD_RUNNABLE, or SD_RUNNING",
           SD_task_get_name(src));

  state = SD_task_get_state(dst);
  if (state != SD_NOT_SCHEDULED && state != SD_SCHEDULABLE && state != SD_SCHEDULED && state != SD_RUNNABLE)
    THROWF(arg_error, 0, "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULABLE, SD_SCHEDULED, or SD_RUNNABLE",
           SD_task_get_name(dst));

  XBT_DEBUG("SD_task_dependency_add: src = %s, dst = %s", SD_task_get_name(src), SD_task_get_name(dst));
  for (unsigned long i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(src->tasks_after, i, &dependency);
    found = (dependency->dst == dst);
    XBT_DEBUG("Dependency %lu: dependency->dst = %s", i, SD_task_get_name(dependency->dst));
  }

  if (found)
    THROWF(arg_error, 0, "A dependency already exists between task '%s' and task '%s'",
           SD_task_get_name(src), SD_task_get_name(dst));

  dependency = xbt_new(s_SD_dependency_t, 1);

  dependency->name = xbt_strdup(name);  /* xbt_strdup is cleaver enough to deal with NULL args itself */
  dependency->data = data;
  dependency->src = src;
  dependency->dst = dst;

  /* src must be executed before dst */
  xbt_dynar_push(src->tasks_after, &dependency);
  xbt_dynar_push(dst->tasks_before, &dependency);

  dst->unsatisfied_dependencies++;
  dst->is_not_ready++;

  /* if the task was runnable, then dst->tasks_before is not empty anymore, so we must go back to state SD_SCHEDULED */
  if (SD_task_get_state(dst) == SD_RUNNABLE) {
    XBT_DEBUG("SD_task_dependency_add: %s was runnable and becomes scheduled!", SD_task_get_name(dst));
    SD_task_set_state(dst, SD_SCHEDULED);
  }
}

/**
 * \brief Returns the name given as input when dependency has been created..
 *
 * \param src a task
 * \param dst a task depending on \a src
 *
 */
const char *SD_task_dependency_get_name(SD_task_t src, SD_task_t dst){
  unsigned int i;
  SD_dependency_t dependency;

  xbt_dynar_foreach(src->tasks_after, i, dependency){
    if (dependency->dst == dst)
      return dependency->name;
  }
  return NULL;
}

/**
 * \brief Indicates whether there is a dependency between two tasks.
 *
 * \param src a task
 * \param dst a task depending on \a src
 *
 * If src is NULL, checks whether dst has any pre-dependency.
 * If dst is NULL, checks whether src has any post-dependency.
 */
int SD_task_dependency_exists(SD_task_t src, SD_task_t dst)
{
  xbt_assert(src != NULL || dst != NULL, "Invalid parameter: both src and dst are NULL");

  if (src) {
    if (dst) {
      unsigned int counter;
      SD_dependency_t dependency;
      xbt_dynar_foreach(src->tasks_after, counter, dependency) {
        if (dependency->dst == dst)
          return 1;
      }
    } else {
      return xbt_dynar_length(src->tasks_after);
    }
  } else {
    return xbt_dynar_length(dst->tasks_before);
  }
  return 0;
}

/**
 * \brief Remove a dependency between two tasks
 *
 * \param src a task
 * \param dst a task depending on \a src
 * \see SD_task_dependency_add()
 */
void SD_task_dependency_remove(SD_task_t src, SD_task_t dst)
{
  unsigned long length;
  int found = 0;
  SD_dependency_t dependency;

  /* remove the dependency from src->tasks_after */
  length = xbt_dynar_length(src->tasks_after);

  for (unsigned long i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(src->tasks_after, i, &dependency);
    if (dependency->dst == dst) {
      xbt_dynar_remove_at(src->tasks_after, i, NULL);
      found = 1;
    }
  }
  if (!found)
    THROWF(arg_error, 0, "No dependency found between task '%s' and '%s': task '%s' is not a successor of task '%s'",
           SD_task_get_name(src), SD_task_get_name(dst), SD_task_get_name(dst), SD_task_get_name(src));

  /* remove the dependency from dst->tasks_before */
  length = xbt_dynar_length(dst->tasks_before);
  found = 0;

  for (unsigned long i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dst->tasks_before, i, &dependency);
    if (dependency->src == src) {
      xbt_dynar_remove_at(dst->tasks_before, i, NULL);
      __SD_task_dependency_destroy(dependency);
      dst->unsatisfied_dependencies--;
      dst->is_not_ready--;
      found = 1;
    }
  }
  /* should never happen... */
  xbt_assert(found, "SimDag error: task '%s' is a successor of '%s' but task '%s' is not a predecessor of task '%s'",
              SD_task_get_name(dst), SD_task_get_name(src), SD_task_get_name(src), SD_task_get_name(dst));

  /* if the task was scheduled and dst->tasks_before is empty now, we can make it runnable */
  if (dst->unsatisfied_dependencies == 0) {
    if (SD_task_get_state(dst) == SD_SCHEDULED)
      SD_task_set_state(dst, SD_RUNNABLE);
    else
      SD_task_set_state(dst, SD_SCHEDULABLE);
  }

  if (dst->is_not_ready == 0)
    SD_task_set_state(dst, SD_SCHEDULABLE);
}

/**
 * \brief Returns the user data associated with a dependency between two tasks
 *
 * \param src a task
 * \param dst a task depending on \a src
 * \return the user data associated with this dependency (can be \c NULL)
 * \see SD_task_dependency_add()
 */
void *SD_task_dependency_get_data(SD_task_t src, SD_task_t dst)
{
  int found = 0;
  SD_dependency_t dependency;

  unsigned long length = xbt_dynar_length(src->tasks_after);

  for (unsigned long i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(src->tasks_after, i, &dependency);
    found = (dependency->dst == dst);
  }
  if (!found)
    THROWF(arg_error, 0, "No dependency found between task '%s' and '%s'",
           SD_task_get_name(src), SD_task_get_name(dst));
  return dependency->data;
}

/**
 * \brief Adds a watch point to a task
 *
 * SD_simulate() will stop as soon as the \ref e_SD_task_state_t "state" of this task becomes the one given in argument.
 * The watch point is then automatically removed.
 *
 * \param task a task
 * \param state the \ref e_SD_task_state_t "state" you want to watch (cannot be #SD_NOT_SCHEDULED)
 * \see SD_task_unwatch()
 */
void SD_task_watch(SD_task_t task, e_SD_task_state_t state)
{
  if (state & SD_NOT_SCHEDULED)
    THROWF(arg_error, 0, "Cannot add a watch point for state SD_NOT_SCHEDULED");

  task->watch_points = task->watch_points | state;
}

/**
 * \brief Removes a watch point from a task
 *
 * \param task a task
 * \param state the \ref e_SD_task_state_t "state" you no longer want to watch
 * \see SD_task_watch()
 */
void SD_task_unwatch(SD_task_t task, e_SD_task_state_t state)
{
  xbt_assert(state != SD_NOT_SCHEDULED, "SimDag error: Cannot have a watch point for state SD_NOT_SCHEDULED");
  task->watch_points = task->watch_points & ~state;
}

/**
 * \brief Returns an approximative estimation of the execution time of a task.
 *
 * The estimation is very approximative because the value returned is the time the task would take if it was executed
 * now and if it was the only task.
 *
 * \param task the task to evaluate
 * \param workstation_nb number of workstations on which the task would be executed
 * \param workstation_list the workstations on which the task would be executed
 * \param flops_amount computation amount for each workstation
 * \param bytes_amount communication amount between each pair of workstations
 * \see SD_schedule()
 */
double SD_task_get_execution_time(SD_task_t task, int workstation_nb, const sg_host_t *workstation_list,
                                  const double *flops_amount, const double *bytes_amount)
{
  xbt_assert(workstation_nb > 0, "Invalid parameter");
  double max_time = 0.0;

  /* the task execution time is the maximum execution time of the parallel tasks */
  for (int i = 0; i < workstation_nb; i++) {
    double time = 0.0;
    if (flops_amount != NULL)
      time = flops_amount[i] / sg_host_speed(workstation_list[i]);

    if (bytes_amount != NULL)
      for (int j = 0; j < workstation_nb; j++) {
        if (bytes_amount[i * workstation_nb + j] !=0 ) {
          time += (SD_route_get_latency(workstation_list[i], workstation_list[j]) +
                   bytes_amount[i * workstation_nb + j] /
                   SD_route_get_bandwidth(workstation_list[i], workstation_list[j]));
        }
      }

    if (time > max_time) {
      max_time = time;
    }
  }
  return max_time;
}

static inline void SD_task_do_schedule(SD_task_t task)
{
  if (SD_task_get_state(task) > SD_SCHEDULABLE)
    THROWF(arg_error, 0, "Task '%s' has already been scheduled", SD_task_get_name(task));

  if (task->unsatisfied_dependencies == 0)
    SD_task_set_state(task, SD_RUNNABLE);
  else
    SD_task_set_state(task, SD_SCHEDULED);
}

/**
 * \brief Schedules a task
 *
 * The task state must be #SD_NOT_SCHEDULED.
 * Once scheduled, a task is executed as soon as possible in \see SD_simulate, i.e. when its dependencies are satisfied.
 *
 * \param task the task you want to schedule
 * \param host_count number of workstations on which the task will be executed
 * \param workstation_list the workstations on which the task will be executed
 * \param flops_amount computation amount for each workstation
 * \param bytes_amount communication amount between each pair of workstations
 * \param rate task execution speed rate
 * \see SD_task_unschedule()
 */
void SD_task_schedule(SD_task_t task, int host_count, const sg_host_t * workstation_list,
                      const double *flops_amount, const double *bytes_amount, double rate)
{
  xbt_assert(host_count > 0, "workstation_nb must be positive");

  task->host_count = host_count;
  task->rate = rate;

  if (flops_amount) {
    task->flops_amount = (double*)xbt_realloc(task->flops_amount, sizeof(double) * host_count);
    memcpy(task->flops_amount, flops_amount, sizeof(double) * host_count);
  } else {
    xbt_free(task->flops_amount);
    task->flops_amount = NULL;
  }

  int communication_nb = host_count * host_count;
  if (bytes_amount) {
    task->bytes_amount = (double*)xbt_realloc(task->bytes_amount, sizeof(double) * communication_nb);
    memcpy(task->bytes_amount, bytes_amount, sizeof(double) * communication_nb);
  } else {
    xbt_free(task->bytes_amount);
    task->bytes_amount = NULL;
  }

  task->host_list = (sg_host_t*) xbt_realloc(task->host_list, sizeof(sg_host_t) * host_count);
  memcpy(task->host_list, workstation_list, sizeof(sg_host_t) * host_count);

  SD_task_do_schedule(task);
}

/**
 * \brief Unschedules a task
 *
 * The task state must be #SD_SCHEDULED, #SD_RUNNABLE, #SD_RUNNING or #SD_FAILED.
 * If you call this function, the task state becomes #SD_NOT_SCHEDULED.
 * Call SD_task_schedule() to schedule it again.
 *
 * \param task the task you want to unschedule
 * \see SD_task_schedule()
 */
void SD_task_unschedule(SD_task_t task)
{
  if (task->state != SD_SCHEDULED && task->state != SD_RUNNABLE && task->state != SD_RUNNING &&
      task->state != SD_FAILED)
    THROWF(arg_error, 0, "Task %s: the state must be SD_SCHEDULED, SD_RUNNABLE, SD_RUNNING or SD_FAILED",
           SD_task_get_name(task));

  if ((task->state == SD_SCHEDULED || task->state == SD_RUNNABLE)
      /* if the task is scheduled or runnable */
      && ((task->kind == SD_TASK_COMP_PAR_AMDAHL) || (task->kind == SD_TASK_COMM_PAR_MXN_1D_BLOCK))) {
          /* Don't free scheduling data for typed tasks */
    __SD_task_destroy_scheduling_data(task);
    xbt_free(task->host_list);
    task->host_list=NULL;
    task->host_count = 0;
  }

  if (SD_task_get_state(task) == SD_RUNNING)
    /* the task should become SD_FAILED */
    task->surf_action->cancel();
  else {
    if (task->unsatisfied_dependencies == 0)
      SD_task_set_state(task, SD_SCHEDULABLE);
    else
      SD_task_set_state(task, SD_NOT_SCHEDULED);
  }
  task->remains = task->amount;
  task->start_time = -1.0;
}

/* Runs a task. */
void SD_task_run(SD_task_t task)
{
  xbt_assert(SD_task_get_state(task) == SD_RUNNABLE, "Task '%s' is not runnable! Task state: %d",
             SD_task_get_name(task), (int)SD_task_get_state(task));
  xbt_assert(task->host_list != NULL, "Task '%s': workstation_list is NULL!", SD_task_get_name(task));

  XBT_DEBUG("Running task '%s'", SD_task_get_name(task));

  /* Copy the elements of the task into the action */
  int host_nb = task->host_count;
  sg_host_t *hosts = xbt_new(sg_host_t, host_nb);

  for (int i = 0; i < host_nb; i++)
    hosts[i] =  task->host_list[i];

  double *flops_amount = xbt_new0(double, host_nb);
  double *bytes_amount = xbt_new0(double, host_nb * host_nb);

  if(task->flops_amount)
    memcpy(flops_amount, task->flops_amount, sizeof(double) * host_nb);
  if(task->bytes_amount)
    memcpy(bytes_amount, task->bytes_amount, sizeof(double) * host_nb * host_nb);

  task->surf_action = surf_host_model->executeParallelTask(host_nb, hosts, flops_amount, bytes_amount, task->rate);

  task->surf_action->setData(task);

  XBT_DEBUG("surf_action = %p", task->surf_action);

  if (task->category)
    TRACE_surf_action(task->surf_action, task->category);

  __SD_task_destroy_scheduling_data(task);      /* now the scheduling data are not useful anymore */
  SD_task_set_state(task, SD_RUNNING);
}

/**
 * \brief Returns the start time of a task
 *
 * The task state must be SD_RUNNING, SD_DONE or SD_FAILED.
 *
 * \param task: a task
 * \return the start time of this task
 */
double SD_task_get_start_time(SD_task_t task)
{
  if (task->surf_action)
    return task->surf_action->getStartTime();
  else
    return task->start_time;
}

/**
 * \brief Returns the finish time of a task
 *
 * The task state must be SD_RUNNING, SD_DONE or SD_FAILED.
 * If the state is not completed yet, the returned value is an estimation of the task finish time. This value can
 * vary until the task is completed.
 *
 * \param task: a task
 * \return the start time of this task
 */
double SD_task_get_finish_time(SD_task_t task)
{
  if (task->surf_action)        /* should never happen as actions are destroyed right after their completion */
    return task->surf_action->getFinishTime();
  else
    return task->finish_time;
}

void SD_task_distribute_comp_amdahl(SD_task_t task, int ws_count)
{
  xbt_assert(task->kind == SD_TASK_COMP_PAR_AMDAHL, "Task %s is not a SD_TASK_COMP_PAR_AMDAHL typed task."
              "Cannot use this function.", SD_task_get_name(task));
  task->flops_amount = xbt_new0(double, ws_count);
  task->bytes_amount = xbt_new0(double, ws_count * ws_count);
  xbt_free(task->host_list);
  task->host_count = ws_count;
  task->host_list = xbt_new0(sg_host_t, ws_count);
  
  for(int i=0;i<ws_count;i++){
    task->flops_amount[i] = (task->alpha + (1 - task->alpha)/ws_count) * task->amount;
  }
} 


/** @brief Auto-schedules a task.
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This allows to specify the task costs at
 * creation, and decouple them from the scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * To be auto-schedulable, a task must be type and created with one of the specialized creation functions.
 *
 * @todo
 * We should create tasks kind for the following categories:
 *  - Point to point communication (done)
 *  - Sequential computation       (done)
 *  - group communication (redistribution, several kinds)
 *  - parallel tasks with no internal communication (one kind per speedup    model such as Amdahl)
 *  - idem+ internal communication. Task type not enough since we cannot store comm cost alongside to comp one)
 */
void SD_task_schedulev(SD_task_t task, int count, const sg_host_t * list)
{
  int i, j;
  SD_dependency_t dep;
  unsigned int cpt;
  xbt_assert(task->kind != 0, "Task %s is not typed. Cannot automatically schedule it.", SD_task_get_name(task));
  switch (task->kind) {
  case SD_TASK_COMP_PAR_AMDAHL:
    SD_task_distribute_comp_amdahl(task, count);
  case SD_TASK_COMM_E2E:
  case SD_TASK_COMP_SEQ:
    xbt_assert(task->host_count == count, "Got %d locations, but were expecting %d locations", count,task->host_count);
    for (i = 0; i < count; i++)
      task->host_list[i] = list[i];
    if (SD_task_get_kind(task)== SD_TASK_COMP_SEQ && !task->flops_amount){
      /*This task has failed and is rescheduled. Reset the flops_amount*/
      task->flops_amount = xbt_new0(double, 1);
      task->flops_amount[0] = task->remains;
    }
    SD_task_do_schedule(task);
    break;
  default:
    xbt_die("Kind of task %s not supported by SD_task_schedulev()", SD_task_get_name(task));
  }

  if (task->kind == SD_TASK_COMM_E2E) {
    XBT_VERB("Schedule comm task %s between %s -> %s. It costs %.f bytes", SD_task_get_name(task),
          sg_host_get_name(task->host_list[0]), sg_host_get_name(task->host_list[1]), task->bytes_amount[2]);
  }

  /* Iterate over all children and parents being COMM_E2E to say where I am located (and start them if runnable) */
  if (task->kind == SD_TASK_COMP_SEQ) {
    XBT_VERB("Schedule computation task %s on %s. It costs %.f flops", SD_task_get_name(task),
          sg_host_get_name(task->host_list[0]), task->flops_amount[0]);

    xbt_dynar_foreach(task->tasks_before, cpt, dep) {
      SD_task_t before = dep->src;
      if (before->kind == SD_TASK_COMM_E2E) {
        before->host_list[1] = task->host_list[0];

        if (before->host_list[0] && (SD_task_get_state(before) < SD_SCHEDULED)) {
          SD_task_do_schedule(before);
          XBT_VERB ("Auto-Schedule comm task %s between %s -> %s. It costs %.f bytes", SD_task_get_name(before),
               sg_host_get_name(before->host_list[0]), sg_host_get_name(before->host_list[1]), before->bytes_amount[2]);
        }
      }
    }
    xbt_dynar_foreach(task->tasks_after, cpt, dep) {
      SD_task_t after = dep->dst;
      if (after->kind == SD_TASK_COMM_E2E) {
        after->host_list[0] = task->host_list[0];
        if (after->host_list[1] && (SD_task_get_state(after) < SD_SCHEDULED)) {
          SD_task_do_schedule(after);
          XBT_VERB ("Auto-Schedule comm task %s between %s -> %s. It costs %.f bytes", SD_task_get_name(after),
               sg_host_get_name(after->host_list[0]), sg_host_get_name(after->host_list[1]), after->bytes_amount[2]);
        }
      }
    }
  }
  /* Iterate over all children and parents being MXN_1D_BLOCK to say where I am located (and start them if runnable) */
  if (task->kind == SD_TASK_COMP_PAR_AMDAHL) {
    XBT_VERB("Schedule computation task %s on %d workstations. %.f flops will be distributed following Amdahl's Law",
          SD_task_get_name(task), task->host_count, task->flops_amount[0]);
    xbt_dynar_foreach(task->tasks_before, cpt, dep) {
      SD_task_t before = dep->src;
      if (before->kind == SD_TASK_COMM_PAR_MXN_1D_BLOCK){
        if (!before->host_list){
          XBT_VERB("Sender side of Task %s is not scheduled yet", SD_task_get_name(before));
          before->host_list = xbt_new0(sg_host_t, count);
          before->host_count = count;
          XBT_VERB("Fill the workstation list with list of Task '%s'", SD_task_get_name(task));
          for (i=0;i<count;i++)
            before->host_list[i] = task->host_list[i];
        } else {
          XBT_VERB("Build communication matrix for task '%s'", SD_task_get_name(before));
          int src_nb, dst_nb;
          double src_start, src_end, dst_start, dst_end;
          src_nb = before->host_count;
          dst_nb = count;
          before->host_list = (sg_host_t*) xbt_realloc(before->host_list, (before->host_count+count)*sizeof(sg_host_t));
          for(i=0; i<count; i++)
            before->host_list[before->host_count+i] = task->host_list[i];

          before->host_count += count;
          xbt_free(before->flops_amount);
          xbt_free(before->bytes_amount);
          before->flops_amount = xbt_new0(double, before->host_count);
          before->bytes_amount = xbt_new0(double, before->host_count* before->host_count);

          for(i=0;i<src_nb;i++){
            src_start = i*before->amount/src_nb;
            src_end = src_start + before->amount/src_nb;
            for(j=0; j<dst_nb; j++){
              dst_start = j*before->amount/dst_nb;
              dst_end = dst_start + before->amount/dst_nb;
              XBT_VERB("(%s->%s): (%.2f, %.2f)-> (%.2f, %.2f)", sg_host_get_name(before->host_list[i]),
                  sg_host_get_name(before->host_list[src_nb+j]), src_start, src_end, dst_start, dst_end);
              if ((src_end <= dst_start) || (dst_end <= src_start)) {
                before->bytes_amount[i*(src_nb+dst_nb)+src_nb+j]=0.0;
              } else {
                before->bytes_amount[i*(src_nb+dst_nb)+src_nb+j] = MIN(src_end, dst_end) - MAX(src_start, dst_start);
              }
              XBT_VERB("==> %.2f", before->bytes_amount[i*(src_nb+dst_nb)+src_nb+j]);
            }
          }

          if (SD_task_get_state(before)< SD_SCHEDULED) {
            SD_task_do_schedule(before);
            XBT_VERB
              ("Auto-Schedule redistribution task %s. Send %.f bytes from %d hosts to %d hosts.",
                  SD_task_get_name(before),before->amount, src_nb, dst_nb);
            }
        }
      }
    }
    xbt_dynar_foreach(task->tasks_after, cpt, dep) {
      SD_task_t after = dep->dst;
      if (after->kind == SD_TASK_COMM_PAR_MXN_1D_BLOCK){
        if (!after->host_list){
          XBT_VERB("Receiver side of Task '%s' is not scheduled yet", SD_task_get_name(after));
          after->host_list = xbt_new0(sg_host_t, count);
          after->host_count = count;
          XBT_VERB("Fill the workstation list with list of Task '%s'", SD_task_get_name(task));
          for (i=0;i<count;i++)
            after->host_list[i] = task->host_list[i];
        } else {
          int src_nb, dst_nb;
          double src_start, src_end, dst_start, dst_end;
          src_nb = count;
          dst_nb = after->host_count;
          after->host_list = (sg_host_t*) xbt_realloc(after->host_list, (after->host_count+count)*sizeof(sg_host_t));
          for(i=after->host_count - 1; i>=0; i--)
            after->host_list[count+i] = after->host_list[i];
          for(i=0; i<count; i++)
            after->host_list[i] = task->host_list[i];

          after->host_count += count;

          xbt_free(after->flops_amount);
          xbt_free(after->bytes_amount);

          after->flops_amount = xbt_new0(double, after->host_count);
          after->bytes_amount = xbt_new0(double, after->host_count* after->host_count);

          for(i=0;i<src_nb;i++){
            src_start = i*after->amount/src_nb;
            src_end = src_start + after->amount/src_nb;
            for(j=0; j<dst_nb; j++){
              dst_start = j*after->amount/dst_nb;
              dst_end = dst_start + after->amount/dst_nb;
              XBT_VERB("(%d->%d): (%.2f, %.2f)-> (%.2f, %.2f)", i, j, src_start, src_end, dst_start, dst_end);
              if ((src_end <= dst_start) || (dst_end <= src_start)) {
                after->bytes_amount[i*(src_nb+dst_nb)+src_nb+j]=0.0;
              } else {
                after->bytes_amount[i*(src_nb+dst_nb)+src_nb+j] = MIN(src_end, dst_end)- MAX(src_start, dst_start);
              }
              XBT_VERB("==> %.2f", after->bytes_amount[i*(src_nb+dst_nb)+src_nb+j]);
            }
          }

          if (SD_task_get_state(after)< SD_SCHEDULED) {
            SD_task_do_schedule(after);
            XBT_VERB ("Auto-Schedule redistribution task %s. Send %.f bytes from %d hosts to %d hosts.",
              SD_task_get_name(after),after->amount, src_nb, dst_nb);
          }
        }
      }
    }
  }
}

/** @brief autoschedule a task on a list of workstations
 *
 * This function is very similar to SD_task_schedulev(), but takes the list of workstations to schedule onto as
 * separate parameters.
 * It builds a proper vector of workstations and then call SD_task_schedulev()
 */
void SD_task_schedulel(SD_task_t task, int count, ...)
{
  va_list ap;
  sg_host_t *list = xbt_new(sg_host_t, count);
  va_start(ap, count);
  for (int i = 0; i < count; i++) {
    list[i] = va_arg(ap, sg_host_t);
  }
  va_end(ap);
  SD_task_schedulev(task, count, list);
  free(list);
}
