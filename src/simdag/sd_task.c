/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simdag/simdag.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_task, sd,
                                "Logging specific to SimDag (task)");

static void __SD_task_remove_dependencies(SD_task_t task);
static void __SD_task_destroy_scheduling_data(SD_task_t task);

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

  SD_task_t task;
  SD_CHECK_INIT_DONE();

  task = xbt_new(s_SD_task_t, 1);

  /* general information */
  task->data = data;            /* user data */
  task->name = xbt_strdup(name);
  task->kind = SD_TASK_NOT_TYPED;
  task->state_hookup.prev = NULL;
  task->state_hookup.next = NULL;
  task->state_set = sd_global->not_scheduled_task_set;
  task->state = SD_NOT_SCHEDULED;
  xbt_swag_insert(task, task->state_set);

  task->amount = amount;
  task->remains = amount;
  task->start_time = -1.0;
  task->finish_time = -1.0;
  task->surf_action = NULL;
  task->watch_points = 0;

  /* dependencies */
  task->tasks_before = xbt_dynar_new(sizeof(SD_dependency_t), NULL);
  task->tasks_after = xbt_dynar_new(sizeof(SD_dependency_t), NULL);

  /* scheduling parameters */
  task->workstation_nb = 0;
  task->workstation_list = NULL;
  task->computation_amount = NULL;
  task->communication_amount = NULL;
  task->rate = 0;

  sd_global->task_number++;

  return task;
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  task->data = data;
}

/**
 * \brief Returns the state of a task
 *
 * \param task a task
 * \return the current \ref e_SD_task_state_t "state" of this task:
 * #SD_NOT_SCHEDULED, #SD_SCHEDULED, #SD_READY, #SD_RUNNING, #SD_DONE or #SD_FAILED
 * \see e_SD_task_state_t
 */
e_SD_task_state_t SD_task_get_state(SD_task_t task)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  return task->state;
}

/* Changes the state of a task. Updates the swags and the flag sd_global->watch_point_reached.
 */
void __SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state)
{
  xbt_swag_remove(task, task->state_set);
  switch (new_state) {
  case SD_NOT_SCHEDULED:
    task->state_set = sd_global->not_scheduled_task_set;
    break;
  case SD_SCHEDULED:
    task->state_set = sd_global->scheduled_task_set;
    break;
  case SD_READY:
    task->state_set = sd_global->ready_task_set;
    break;
  case SD_IN_FIFO:
    task->state_set = sd_global->in_fifo_task_set;
    break;
  case SD_RUNNING:
    task->state_set = sd_global->running_task_set;
    task->start_time =
      surf_workstation_model->action_get_start_time(task->surf_action);
    break;
  case SD_DONE:
    task->state_set = sd_global->done_task_set;
    task->finish_time =
      surf_workstation_model->action_get_finish_time(task->surf_action);
    task->remains = 0;
    break;
  case SD_FAILED:
    task->state_set = sd_global->failed_task_set;
    break;
  default:
    xbt_assert0(0, "Invalid state");
  }
  xbt_swag_insert(task, task->state_set);
  task->state = new_state;

  if (task->watch_points & new_state) {
    INFO1("Watch point reached with task '%s'!", SD_task_get_name(task));
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  return task->name;
}

/** @brief Allows to change the name of a task */
void SD_task_set_name(SD_task_t task, const char *name) {
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
  xbt_dynar_t parents;
  SD_dependency_t dep;
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  parents = xbt_dynar_new(sizeof(SD_task_t), NULL);
  xbt_dynar_foreach(task->tasks_before, i, dep){
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
  xbt_dynar_t children;
  SD_dependency_t dep;
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  children = xbt_dynar_new(sizeof(SD_task_t), NULL);
  xbt_dynar_foreach(task->tasks_after, i, dep){
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  //  xbt_assert1( task->state_set != sd_global->scheduled_task_set, 
  //	       "Unscheduled task %s", task->name);
  return task->workstation_nb;
}

/**
 * \brief Returns the list of workstations involved in a task
 *
 * Only call this on already scheduled tasks!
 * \param task a task
 */
SD_workstation_t* SD_task_get_workstation_list(SD_task_t task)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  //xbt_assert1( task->state_set != sd_global->scheduled_task_set, 
  //	       "Unscheduled task %s", task->name);
  return task->workstation_list;
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  return task->amount;
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  if (task->surf_action)
    return surf_workstation_model->get_remains(task->surf_action);
  else
    return task->remains;
}

int SD_task_get_kind(SD_task_t task) {
  return task->kind;
}

/** @brief Displays debugging informations about a task */
void SD_task_dump(SD_task_t task)
{
  unsigned int counter;
  SD_dependency_t dependency;
  char *statename;

  INFO1("Displaying task %s",SD_task_get_name(task));
  statename=bprintf("%s %s %s %s %s %s %s",
      (task->state&SD_NOT_SCHEDULED?"not scheduled":""),
      (task->state&SD_SCHEDULED?"scheduled":""),
      (task->state&SD_READY?"ready":"not ready"),
      (task->state&SD_IN_FIFO?"in fifo":""),
      (task->state&SD_RUNNING?"running":""),
      (task->state&SD_DONE?"done":""),
      (task->state&SD_FAILED?"failed":""));
  INFO1("  - state: %s",statename);
  free(statename);

  if (task->kind!=0) {
    switch(task->kind){
    case SD_TASK_COMM_E2E:
      INFO0("  - kind: end-to-end communication");
      break;
    case SD_TASK_COMP_SEQ:
      INFO0("  - kind: sequential computation");
      break;
    default:
      INFO1("  - (unknown kind %d)",task->kind);
    }
  }
  INFO1("  - amount: %.0f",SD_task_get_amount(task));
  if (xbt_dynar_length(task->tasks_before)) {
    INFO0("  - pre-dependencies:");
    xbt_dynar_foreach(task->tasks_before,counter,dependency) {
      INFO1("    %s",SD_task_get_name(dependency->src));
    }
  }
  if (xbt_dynar_length(task->tasks_after)) {
    INFO0("  - post-dependencies:");
    xbt_dynar_foreach(task->tasks_after,counter,dependency) {
      INFO1("    %s",SD_task_get_name(dependency->dst));
    }
  }
}
/** @brief Dumps the task in dotty formalism into the FILE* passed as second argument */
void SD_task_dotty(SD_task_t task,void* out) {
  unsigned int counter;
  SD_dependency_t dependency;
  fprintf(out, "  T%p [label=\"%.20s\"",task, task->name);
  switch(task->kind){
    case SD_TASK_COMM_E2E:
      fprintf(out,", shape=box");
      break;
    case SD_TASK_COMP_SEQ:
      fprintf(out,", shape=circle");
      break;
    default:
      xbt_die("Unknown task type!");
  }
  fprintf(out,"];\n");
  xbt_dynar_foreach(task->tasks_before,counter,dependency) {
    fprintf(out," T%p -> T%p;\n",dependency->src, dependency->dst);
  }
}

/* Destroys a dependency between two tasks.
 */
static void __SD_task_dependency_destroy(void *dependency)
{
  if (((SD_dependency_t) dependency)->name != NULL)
    xbt_free(((SD_dependency_t) dependency)->name);
  xbt_free(dependency);
}

/**
 * \brief Adds a dependency between two tasks
 *
 * \a dst will depend on \a src, ie \a dst will not start before \a src is finished.
 * Their \ref e_SD_task_state_t "state" must be #SD_NOT_SCHEDULED, #SD_SCHEDULED or #SD_READY.
 *
 * \param name the name of the new dependency (can be \c NULL)
 * \param data the user data you want to associate with this dependency (can be \c NULL)
 * \param src the task which must be executed first
 * \param dst the task you want to make depend on \a src
 * \see SD_task_dependency_remove()
 */
void SD_task_dependency_add(const char *name, void *data, SD_task_t src,
                            SD_task_t dst)
{
  xbt_dynar_t dynar;
  int length;
  int found = 0;
  int i;
  SD_dependency_t dependency;

  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");

  dynar = src->tasks_after;
  length = xbt_dynar_length(dynar);

  if (src == dst)
    THROW1(arg_error, 0,
           "Cannot add a dependency between task '%s' and itself",
           SD_task_get_name(src));

  if (!__SD_task_is_not_scheduled(src)
      && !__SD_task_is_scheduled_or_ready(src))
    THROW1(arg_error, 0,
           "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULED or SD_READY",
           SD_task_get_name(src));

  if (!__SD_task_is_not_scheduled(dst)
      && !__SD_task_is_scheduled_or_ready(dst))
    THROW1(arg_error, 0,
           "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULED or SD_READY",
           SD_task_get_name(dst));

  DEBUG2("SD_task_dependency_add: src = %s, dst = %s", SD_task_get_name(src),
         SD_task_get_name(dst));
  for (i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    found = (dependency->dst == dst);
    DEBUG2("Dependency %d: dependency->dst = %s", i,
           SD_task_get_name(dependency->dst));
  }

  if (found)
    THROW2(arg_error, 0,
           "A dependency already exists between task '%s' and task '%s'",
           SD_task_get_name(src), SD_task_get_name(dst));

  dependency = xbt_new(s_SD_dependency_t, 1);

  dependency->name = xbt_strdup(name); /* xbt_strdup is cleaver enough to deal with NULL args itself */
  dependency->data = data;
  dependency->src = src;
  dependency->dst = dst;

  /* src must be executed before dst */
  xbt_dynar_push(src->tasks_after, &dependency);
  xbt_dynar_push(dst->tasks_before, &dependency);

  /* if the task was ready, then dst->tasks_before is not empty anymore,
     so we must go back to state SD_SCHEDULED */
  if (__SD_task_is_ready(dst)) {
    DEBUG1("SD_task_dependency_add: %s was ready and becomes scheduled!",
           SD_task_get_name(dst));
    __SD_task_set_state(dst, SD_SCHEDULED);
  }

  /*  __SD_print_dependencies(src);
     __SD_print_dependencies(dst); */
}

/**
 * \brief Indacates whether there is a dependency between two tasks.
 *
 * \param src a task
 * \param dst a task depending on \a src
 *
 * If src is NULL, checks whether dst has any pre-dependency.
 * If dst is NULL, checks whether src has any post-dependency.
 */
int SD_task_dependency_exists(SD_task_t src, SD_task_t dst)
{
  unsigned int counter;
  SD_dependency_t dependency;

  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL || dst != NULL, "Invalid parameter: both src and dst are NULL");

  if (src) {
    if (dst) {
      xbt_dynar_foreach(src->tasks_after,counter,dependency) {
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

  xbt_dynar_t dynar;
  int length;
  int found = 0;
  int i;
  SD_dependency_t dependency;

  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");

  /* remove the dependency from src->tasks_after */
  dynar = src->tasks_after;
  length = xbt_dynar_length(dynar);

  for (i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    if (dependency->dst == dst) {
      xbt_dynar_remove_at(dynar, i, NULL);
      found = 1;
    }
  }
  if (!found)
    THROW4(arg_error, 0,
           "No dependency found between task '%s' and '%s': task '%s' is not a successor of task '%s'",
           SD_task_get_name(src), SD_task_get_name(dst),
           SD_task_get_name(dst), SD_task_get_name(src));

  /* remove the dependency from dst->tasks_before */
  dynar = dst->tasks_before;
  length = xbt_dynar_length(dynar);
  found = 0;

  for (i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    if (dependency->src == src) {
      xbt_dynar_remove_at(dynar, i, NULL);
      __SD_task_dependency_destroy(dependency);
      found = 1;
    }
  }
  /* should never happen... */
  xbt_assert4(found,
              "SimDag error: task '%s' is a successor of '%s' but task '%s' is not a predecessor of task '%s'",
              SD_task_get_name(dst), SD_task_get_name(src),
              SD_task_get_name(src), SD_task_get_name(dst));

  /* if the task was scheduled and dst->tasks_before is empty now, we can make it ready */
  if (xbt_dynar_length(dst->tasks_before) == 0 && __SD_task_is_scheduled(dst))
    __SD_task_set_state(dst, SD_READY);

  /*  __SD_print_dependencies(src);
     __SD_print_dependencies(dst); */
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

  xbt_dynar_t dynar;
  int length;
  int found = 0;
  int i;
  SD_dependency_t dependency;


  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");

  dynar = src->tasks_after;
  length = xbt_dynar_length(dynar);

  for (i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    found = (dependency->dst == dst);
  }
  if (!found)
    THROW2(arg_error, 0, "No dependency found between task '%s' and '%s'",
           SD_task_get_name(src), SD_task_get_name(dst));
  return dependency->data;
}

/* temporary function for debugging */
static void __SD_print_watch_points(SD_task_t task)
{
  static const int state_masks[] =
    { SD_SCHEDULED, SD_RUNNING, SD_READY, SD_DONE, SD_FAILED };
  static const char *state_names[] =
    { "scheduled", "running", "ready", "done", "failed" };
  int i;

  INFO2("Task '%s' watch points (%x): ", SD_task_get_name(task),
        task->watch_points);


  for (i = 0; i < 5; i++) {
    if (task->watch_points & state_masks[i])
      INFO1("%s ", state_names[i]);
  }
}

/**
 * \brief Adds a watch point to a task
 *
 * SD_simulate() will stop as soon as the \ref e_SD_task_state_t "state" of this
 * task becomes the one given in argument. The
 * watch point is then automatically removed.
 *
 * \param task a task
 * \param state the \ref e_SD_task_state_t "state" you want to watch
 * (cannot be #SD_NOT_SCHEDULED)
 * \see SD_task_unwatch()
 */
void SD_task_watch(SD_task_t task, e_SD_task_state_t state)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  if (state & SD_NOT_SCHEDULED)
    THROW0(arg_error, 0,
           "Cannot add a watch point for state SD_NOT_SCHEDULED");

  task->watch_points = task->watch_points | state;
  /*  __SD_print_watch_points(task); */
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  xbt_assert0(state != SD_NOT_SCHEDULED,
              "SimDag error: Cannot have a watch point for state SD_NOT_SCHEDULED");

  task->watch_points = task->watch_points & ~state;
  /*  __SD_print_watch_points(task); */
}

/**
 * \brief Returns an approximative estimation of the execution time of a task.
 *
 * The estimation is very approximative because the value returned is the time
 * the task would take if it was executed now and if it was the only task.
 *
 * \param task the task to evaluate
 * \param workstation_nb number of workstations on which the task would be executed
 * \param workstation_list the workstations on which the task would be executed
 * \param computation_amount computation amount for each workstation
 * \param communication_amount communication amount between each pair of workstations
 * \see SD_schedule()
 */
double SD_task_get_execution_time(SD_task_t task,
                                  int workstation_nb,
                                  const SD_workstation_t * workstation_list,
                                  const double *computation_amount,
                                  const double *communication_amount)
{
  double time, max_time = 0.0;
  int i, j;
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL && workstation_nb > 0 && workstation_list != NULL,
              "Invalid parameter");

  /* the task execution time is the maximum execution time of the parallel tasks */

  for (i = 0; i < workstation_nb; i++) {
    time = 0.0;
    if (computation_amount != NULL)
      time =
          SD_workstation_get_computation_time(workstation_list[i],
              computation_amount[i]);

    if (communication_amount != NULL)
      for (j = 0; j < workstation_nb; j++) {
        time +=
            SD_route_get_communication_time(workstation_list[i],
                workstation_list[j],
                communication_amount[i *
                                     workstation_nb +
                                     j]);
      }

    if (time > max_time) {
      max_time = time;
    }
  }
  return max_time;
}
static inline void SD_task_do_schedule(SD_task_t task) {
  SD_CHECK_INIT_DONE();

   if (!__SD_task_is_not_scheduled(task))
     THROW1(arg_error, 0, "Task '%s' has already been scheduled",
            SD_task_get_name(task));

 /* update the task state */
  if (xbt_dynar_length(task->tasks_before) == 0)
    __SD_task_set_state(task, SD_READY);
  else
    __SD_task_set_state(task, SD_SCHEDULED);
}

/**
 * \brief Schedules a task
 *
 * The task state must be #SD_NOT_SCHEDULED.
 * Once scheduled, a task will be executed as soon as possible in SD_simulate(),
 * i.e. when its dependencies are satisfied.
 *
 * \param task the task you want to schedule
 * \param workstation_nb number of workstations on which the task will be executed
 * \param workstation_list the workstations on which the task will be executed
 * \param computation_amount computation amount for each workstation
 * \param communication_amount communication amount between each pair of workstations
 * \param rate task execution speed rate
 * \see SD_task_unschedule()
 */
void SD_task_schedule(SD_task_t task, int workstation_count,
                      const SD_workstation_t * workstation_list,
                      const double *computation_amount,
                      const double *communication_amount, double rate)
{
  xbt_assert0(workstation_count > 0, "workstation_nb must be positive");

  int communication_nb;

  task->workstation_nb = workstation_count;
  task->rate = rate;

  if (computation_amount) {
    task->computation_amount = xbt_new(double, workstation_count);
    memcpy(task->computation_amount, computation_amount,
        sizeof(double) * workstation_count);
  } else {
    task->computation_amount = NULL;
  }

  communication_nb = workstation_count * workstation_count;
  if (communication_amount) {
    task->communication_amount = xbt_new(double, communication_nb);
    memcpy(task->communication_amount, communication_amount,
        sizeof(double) * communication_nb);
  } else {
    task->communication_amount = NULL;
  }

  task->workstation_list = xbt_new(SD_workstation_t, workstation_count);
  memcpy(task->workstation_list, workstation_list,
         sizeof(SD_workstation_t) * workstation_count);

  SD_task_do_schedule(task);
}
/**
 * \brief Unschedules a task
 *
 * The task state must be #SD_SCHEDULED, #SD_READY, #SD_RUNNING or #SD_FAILED.
 * If you call this function, the task state becomes #SD_NOT_SCHEDULED.
 * Call SD_task_schedule() to schedule it again.
 *
 * \param task the task you want to unschedule
 * \see SD_task_schedule()
 */
void SD_task_unschedule(SD_task_t task)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  if (task->state_set != sd_global->scheduled_task_set &&
      task->state_set != sd_global->ready_task_set &&
      task->state_set != sd_global->running_task_set &&
      task->state_set != sd_global->failed_task_set)
    THROW1(arg_error, 0,
           "Task %s: the state must be SD_SCHEDULED, SD_READY, SD_RUNNING or SD_FAILED",
           SD_task_get_name(task));

  if (__SD_task_is_scheduled_or_ready(task) /* if the task is scheduled or ready */
      && task->kind == SD_TASK_NOT_TYPED) /* Don't free scheduling data for typed tasks */
    __SD_task_destroy_scheduling_data(task);

  if (__SD_task_is_running(task))       /* the task should become SD_FAILED */
    surf_workstation_model->action_cancel(task->surf_action);
  else
    __SD_task_set_state(task, SD_NOT_SCHEDULED);
  task->remains = task->amount;
  task->start_time = -1.0;
}

/* Destroys the data memorised by SD_task_schedule. Task state must be SD_SCHEDULED or SD_READY.
 */
static void __SD_task_destroy_scheduling_data(SD_task_t task)
{
  SD_CHECK_INIT_DONE();
  if (!__SD_task_is_scheduled_or_ready(task) && !__SD_task_is_in_fifo(task))
    THROW1(arg_error, 0,
           "Task '%s' must be SD_SCHEDULED, SD_READY or SD_IN_FIFO",
           SD_task_get_name(task));

  xbt_free(task->computation_amount);
  xbt_free(task->communication_amount);
  task->computation_amount = task->communication_amount = NULL;
}

/* Runs a task. This function is directly called by __SD_task_try_to_run if the task
 * doesn't have to wait in fifos. Otherwise, it is called by __SD_task_just_done when
 * the task gets out of its fifos.
 */
void __SD_task_really_run(SD_task_t task)
{

  int i;
  void **surf_workstations;

  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  xbt_assert2(__SD_task_is_ready_or_in_fifo(task),
              "Task '%s' is not ready or in a fifo! Task state: %d",
              SD_task_get_name(task), SD_task_get_state(task));
  xbt_assert1(task->workstation_list != NULL,
              "Task '%s': workstation_list is NULL!", SD_task_get_name(task));



  DEBUG1("Really running task '%s'", SD_task_get_name(task));

  /* set this task as current task for the workstations in sequential mode */
  for (i = 0; i < task->workstation_nb; i++) {
    if (SD_workstation_get_access_mode(task->workstation_list[i]) ==
        SD_WORKSTATION_SEQUENTIAL_ACCESS) {
      task->workstation_list[i]->current_task = task;
      xbt_assert0(__SD_workstation_is_busy(task->workstation_list[i]),
                  "The workstation should be busy now");
    }
  }

  DEBUG1("Task '%s' set as current task for its workstations",
         SD_task_get_name(task));

  /* start the task */

  /* we have to create a Surf workstation array instead of the SimDag workstation array */
  surf_workstations = xbt_new(void *, task->workstation_nb);

  for (i = 0; i < task->workstation_nb; i++)
    surf_workstations[i] = task->workstation_list[i]->surf_workstation;

  /* It's allowed to pass a NULL vector as cost to mean vector of 0.0 (easing user's life). Let's deal with it */
#define cost_or_zero(array,pos) ((array)?(array)[pos]:0.0)

  task->surf_action = NULL;
  if ((task->workstation_nb == 1) && (cost_or_zero(task->communication_amount,0) == 0.0)) {
    task->surf_action =
      surf_workstation_model->extension.
      workstation.execute(surf_workstations[0], cost_or_zero(task->computation_amount,0));
  } else if ((task->workstation_nb == 1)
             && (cost_or_zero(task->computation_amount,0) == 0.0)) {

    task->surf_action =
      surf_workstation_model->extension.
      workstation.communicate(surf_workstations[0], surf_workstations[0],
                              cost_or_zero(task->communication_amount,0), task->rate);
  } else if ((task->workstation_nb == 2)
             && (cost_or_zero(task->computation_amount,0) == 0.0)
             && (cost_or_zero(task->computation_amount,1) == 0.0)) {
    int nb = 0;
    double value = 0.0;

    for (i = 0; i < task->workstation_nb * task->workstation_nb; i++) {
      if (cost_or_zero(task->communication_amount,i) > 0.0) {
        nb++;
        value = cost_or_zero(task->communication_amount,i);
      }
    }
    if (nb == 1) {
      task->surf_action =
        surf_workstation_model->extension.
        workstation.communicate(surf_workstations[0], surf_workstations[1],
                                value, task->rate);
    }
  }
#undef cost_or_zero

  if (!task->surf_action) {
    double *computation_amount = xbt_new(double, task->workstation_nb);
    double *communication_amount = xbt_new(double, task->workstation_nb *
                                           task->workstation_nb);

    memcpy(computation_amount, task->computation_amount, sizeof(double) *
           task->workstation_nb);
    memcpy(communication_amount, task->communication_amount,
           sizeof(double) * task->workstation_nb * task->workstation_nb);

    task->surf_action =
      surf_workstation_model->extension.
      workstation.execute_parallel_task(task->workstation_nb,
                                        surf_workstations, computation_amount,
                                        communication_amount, task->amount,
                                        task->rate);
  } else {
    xbt_free(surf_workstations);
  }

  surf_workstation_model->action_data_set(task->surf_action, task);

  DEBUG1("surf_action = %p", task->surf_action);

  __SD_task_destroy_scheduling_data(task);      /* now the scheduling data are not useful anymore */
  __SD_task_set_state(task, SD_RUNNING);
  xbt_assert2(__SD_task_is_running(task), "Bad state of task '%s': %d",
              SD_task_get_name(task), SD_task_get_state(task));

}

/* Tries to run a task. This function is called by SD_simulate() when a scheduled task becomes SD_READY
 * (ie when its dependencies are satisfied).
 * If one of the workstations where the task is scheduled on is busy (in sequential mode),
 * the task doesn't start.
 * Returns whether the task has started.
 */
int __SD_task_try_to_run(SD_task_t task)
{

  int can_start = 1;
  int i;
  SD_workstation_t workstation;

  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  xbt_assert2(__SD_task_is_ready(task),
              "Task '%s' is not ready! Task state: %d",
              SD_task_get_name(task), SD_task_get_state(task));


  for (i = 0; i < task->workstation_nb; i++) {
    can_start = can_start && 
      !__SD_workstation_is_busy(task->workstation_list[i]);
  }

  DEBUG2("Task '%s' can start: %d", SD_task_get_name(task), can_start);

  if (!can_start) {             /* if the task cannot start and is not in the fifos yet */
    for (i = 0; i < task->workstation_nb; i++) {
      workstation = task->workstation_list[i];
      if (workstation->access_mode == SD_WORKSTATION_SEQUENTIAL_ACCESS) {
        DEBUG2("Pushing task '%s' in the fifo of workstation '%s'",
               SD_task_get_name(task), SD_workstation_get_name(workstation));
        xbt_fifo_push(workstation->task_fifo, task);
      }
    }
    __SD_task_set_state(task, SD_IN_FIFO);
    xbt_assert2(__SD_task_is_in_fifo(task), "Bad state of task '%s': %d",
                SD_task_get_name(task), SD_task_get_state(task));
    DEBUG1("Task '%s' state is now SD_IN_FIFO", SD_task_get_name(task));
  } else {
    __SD_task_really_run(task);
  }

  return can_start;
}

/* This function is called by SD_simulate when a task is done.
 * It updates task->state and task->action and executes if necessary the tasks
 * which were waiting in fifos for the end of `task'
 */
void __SD_task_just_done(SD_task_t task)
{
  int i, j;
  SD_workstation_t workstation;

  SD_task_t candidate;
  int candidate_nb = 0;
  int candidate_capacity = 8;
  SD_task_t *candidates;
  int can_start = 1;

  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  xbt_assert1(__SD_task_is_running(task),
              "The task must be running! Task state: %d",
              SD_task_get_state(task));
  xbt_assert1(task->workstation_list != NULL,
              "Task '%s': workstation_list is NULL!", SD_task_get_name(task));


  candidates = xbt_new(SD_task_t, 8);

  __SD_task_set_state(task, SD_DONE);
  surf_workstation_model->action_unref(task->surf_action);
  task->surf_action = NULL;

  DEBUG0("Looking for candidates");

  /* if the task was executed on sequential workstations,
     maybe we can execute the next task of the fifo for each workstation */
  for (i = 0; i < task->workstation_nb; i++) {
    workstation = task->workstation_list[i];
    DEBUG2("Workstation '%s': access_mode = %d",
           SD_workstation_get_name(workstation), workstation->access_mode);
    if (workstation->access_mode == SD_WORKSTATION_SEQUENTIAL_ACCESS) {
      xbt_assert1(workstation->task_fifo != NULL,
                  "Workstation '%s' has sequential access but no fifo!",
                  SD_workstation_get_name(workstation));
      xbt_assert2(workstation->current_task =
                  task, "Workstation '%s': current task should be '%s'",
                  SD_workstation_get_name(workstation),
                  SD_task_get_name(task));

      /* the task is over so we can release the workstation */
      workstation->current_task = NULL;

      DEBUG0("Getting candidate in fifo");
      candidate =
        xbt_fifo_get_item_content(xbt_fifo_get_first_item
                                  (workstation->task_fifo));

      if (candidate != NULL) {
        DEBUG1("Candidate: '%s'", SD_task_get_name(candidate));
        xbt_assert2(__SD_task_is_in_fifo(candidate),
                    "Bad state of candidate '%s': %d",
                    SD_task_get_name(candidate),
                    SD_task_get_state(candidate));
      }

      DEBUG1("Candidate in fifo: %p", candidate);

      /* if there was a task waiting for my place */
      if (candidate != NULL) {
        /* Unfortunately, we are not sure yet that we can execute the task now,
           because the task can be waiting more deeply in some other workstation's fifos...
           So we memorize all candidate tasks, and then we will check for each candidate
           whether or not all its workstations are available. */

        /* realloc if necessary */
        if (candidate_nb == candidate_capacity) {
          candidate_capacity *= 2;
          candidates =
            xbt_realloc(candidates, sizeof(SD_task_t) * candidate_capacity);
        }

        /* register the candidate */
        candidates[candidate_nb++] = candidate;
        candidate->fifo_checked = 0;
      }
    }
  }

  DEBUG1("Candidates found: %d", candidate_nb);

  /* now we check every candidate task */
  for (i = 0; i < candidate_nb; i++) {
    candidate = candidates[i];

    if (candidate->fifo_checked) {
      continue;                 /* we have already evaluated that task */
    }

    xbt_assert2(__SD_task_is_in_fifo(candidate),
                "Bad state of candidate '%s': %d",
                SD_task_get_name(candidate), SD_task_get_state(candidate));

    for (j = 0; j < candidate->workstation_nb && can_start; j++) {
      workstation = candidate->workstation_list[j];

      /* I can start on this workstation if the workstation is shared
         or if I am the first task in the fifo */
      can_start = workstation->access_mode == SD_WORKSTATION_SHARED_ACCESS ||
        candidate ==
        xbt_fifo_get_item_content(xbt_fifo_get_first_item
                                  (workstation->task_fifo));
    }

    DEBUG2("Candidate '%s' can start: %d", SD_task_get_name(candidate),
           can_start);

    /* now we are sure that I can start! */
    if (can_start) {
      for (j = 0; j < candidate->workstation_nb && can_start; j++) {
        workstation = candidate->workstation_list[j];

        /* update the fifo */
        if (workstation->access_mode == SD_WORKSTATION_SEQUENTIAL_ACCESS) {
          candidate = xbt_fifo_shift(workstation->task_fifo);   /* the return value is stored just for debugging */
          DEBUG1("Head of the fifo: '%s'",
                 (candidate != NULL) ? SD_task_get_name(candidate) : "NULL");
          xbt_assert0(candidate == candidates[i],
                      "Error in __SD_task_just_done: bad first task in the fifo");
        }
      }                         /* for each workstation */

      /* finally execute the task */
      DEBUG2("Task '%s' state: %d", SD_task_get_name(candidate),
             SD_task_get_state(candidate));
      __SD_task_really_run(candidate);

      DEBUG4
        ("Calling __SD_task_is_running: task '%s', state set: %p, running_task_set: %p, is running: %d",
         SD_task_get_name(candidate), candidate->state_set,
         sd_global->running_task_set, __SD_task_is_running(candidate));
      xbt_assert2(__SD_task_is_running(candidate),
                  "Bad state of task '%s': %d", SD_task_get_name(candidate),
                  SD_task_get_state(candidate));
      DEBUG0("Okay, the task is running.");

    }                           /* can start */
    candidate->fifo_checked = 1;
  }                             /* for each candidate */

  xbt_free(candidates);
}

/* Remove all dependencies associated with a task. This function is called when the task is destroyed.
 */
static void __SD_task_remove_dependencies(SD_task_t task)
{
  /* we must destroy the dependencies carefuly (with SD_dependency_remove)
     because each one is stored twice */
  SD_dependency_t dependency;
  while (xbt_dynar_length(task->tasks_before) > 0) {
    xbt_dynar_get_cpy(task->tasks_before, 0, &dependency);
    SD_task_dependency_remove(dependency->src, dependency->dst);
  }

  while (xbt_dynar_length(task->tasks_after) > 0) {
    xbt_dynar_get_cpy(task->tasks_after, 0, &dependency);
    SD_task_dependency_remove(dependency->src, dependency->dst);
  }
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  if (task->surf_action)
    return surf_workstation_model->action_get_start_time(task->surf_action);
  else
    return task->start_time;
}

/**
 * \brief Returns the finish time of a task
 *
 * The task state must be SD_RUNNING, SD_DONE or SD_FAILED.
 * If the state is not completed yet, the returned value is an
 * estimation of the task finish time. This value can fluctuate
 * until the task is completed.
 *
 * \param task: a task
 * \return the start time of this task
 */
double SD_task_get_finish_time(SD_task_t task)
{
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  if (task->surf_action)        /* should never happen as actions are destroyed right after their completion */
    return surf_workstation_model->action_get_finish_time(task->surf_action);
  else
    return task->finish_time;
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
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  DEBUG1("Destroying task %s...", SD_task_get_name(task));

  __SD_task_remove_dependencies(task);
  /* if the task was scheduled or ready we have to free the scheduling parameters */
  if (__SD_task_is_scheduled_or_ready(task))
    __SD_task_destroy_scheduling_data(task);
  xbt_swag_remove(task,task->state_set);

  if (task->name != NULL)
    xbt_free(task->name);

  if (task->surf_action != NULL)
    surf_workstation_model->action_unref(task->surf_action);

  if (task->workstation_list != NULL)
    xbt_free(task->workstation_list);

  if (task->communication_amount)
    xbt_free(task->communication_amount);

  if (task->computation_amount)
    xbt_free(task->computation_amount);

  xbt_dynar_free(&task->tasks_before);
  xbt_dynar_free(&task->tasks_after);
  xbt_free(task);

  sd_global->task_number--;

  DEBUG0("Task destroyed.");
}


static inline SD_task_t SD_task_create_sized(const char*name,void*data,double amount,int ws_count) {
  SD_task_t task = SD_task_create(name,data,amount);
  task->communication_amount = xbt_new0(double,ws_count*ws_count);
  task->computation_amount = xbt_new0(double,ws_count);
  task->workstation_nb = ws_count;
  task->workstation_list = xbt_new0(SD_workstation_t,ws_count);
  return task;
}
/** @brief create a end-to-end communication task that can then be auto-scheduled
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This
 * allows to specify the task costs at creation, and decorelate them from the
 * scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * A end-to-end communication must be scheduled on 2 hosts, and the amount
 * specified at creation is sent from hosts[0] to hosts[1].
 */
SD_task_t SD_task_create_comm_e2e(const char*name, void *data, double amount) {
  SD_task_t res = SD_task_create_sized(name,data,amount,2);
  res->communication_amount[2] = amount;
  res->kind=SD_TASK_COMM_E2E;
  return res;
}
/** @brief create a sequential computation task that can then be auto-scheduled
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This
 * allows to specify the task costs at creation, and decorelate them from the
 * scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * A sequential computation must be scheduled on 1 host, and the amount
 * specified at creation to be run on hosts[0].
 */
SD_task_t SD_task_create_comp_seq(const char*name, void *data, double amount) {
  SD_task_t res = SD_task_create_sized(name,data,amount,1);
  res->computation_amount[0]=amount;
  res->kind=SD_TASK_COMP_SEQ;
  return res;
}

/** @brief Auto-schedules a task.
 *
 * Auto-scheduling mean that the task can be used with SD_task_schedulev(). This
 * allows to specify the task costs at creation, and decorelate them from the
 * scheduling process where you just specify which resource should deliver the
 * mandatory power.
 *
 * To be auto-schedulable, a task must be created with SD_task_create_comm_e2e() or
 * SD_task_create_comp_seq(). Check their definitions for the exact semantic of each
 * of them.
 *
 * @todo
 * We should create tasks kind for the following categories:
 *  - Point to point communication (done)
 *  - Sequential computation       (done)
 *  - group communication (redistribution, several kinds)
 *  - parallel tasks with no internal communication (one kind per speedup model such as amdal)
 *  - idem+ internal communication. Task type not enough since we cannot store comm cost alongside to comp one)
 */
void SD_task_schedulev(SD_task_t task, int count, const SD_workstation_t*list) {
  int i;
  xbt_assert1(task->kind != 0,"Task %s is not typed. Cannot automatically schedule it.",SD_task_get_name(task));
  switch(task->kind) {
  case SD_TASK_COMM_E2E:
  case SD_TASK_COMP_SEQ:
    xbt_assert(task->workstation_nb==count);
    for (i=0;i<count;i++)
      task->workstation_list[i]=list[i];
    SD_task_do_schedule(task);
    break;
  default:
    xbt_die(bprintf("Kind of task %s not supported by SD_task_schedulev()",
          SD_task_get_name(task)));
  }
  if (task->kind == SD_TASK_COMM_E2E) {
    VERB4("Schedule comm task %s between %s -> %s. It costs %.f bytes",
        SD_task_get_name(task),
        SD_workstation_get_name(task->workstation_list[0]),SD_workstation_get_name(task->workstation_list[1]),
        task->communication_amount[2]);

  }
  /* Iterate over all childs and parent being COMM_E2E to say where I am located (and start them if ready) */
  if (task->kind == SD_TASK_COMP_SEQ) {
    VERB3("Schedule computation task %s on %s. It costs %.f flops",
        SD_task_get_name(task),SD_workstation_get_name(task->workstation_list[0]),
        task->computation_amount[0]);
    SD_dependency_t dep;
    unsigned int cpt;
    xbt_dynar_foreach(task->tasks_before,cpt,dep) {
      SD_task_t before = dep->src;
      if (before->kind == SD_TASK_COMM_E2E) {
        before->workstation_list[1] = task->workstation_list[0];
        if (before->workstation_list[0] && __SD_task_is_not_scheduled(before)) {
          SD_task_do_schedule(before);
          VERB4("Auto-Schedule comm task %s between %s -> %s. It costs %.f bytes",
              SD_task_get_name(before),
              SD_workstation_get_name(before->workstation_list[0]),SD_workstation_get_name(before->workstation_list[1]),
              before->communication_amount[2]);
        }
      }
    }
    xbt_dynar_foreach(task->tasks_after,cpt,dep) {
      SD_task_t after = dep->dst;
      if (after->kind == SD_TASK_COMM_E2E) {
        after->workstation_list[0] = task->workstation_list[0];
        if (after->workstation_list[1] && __SD_task_is_not_scheduled(after)) {
          SD_task_do_schedule(after);
          VERB4("Auto-Schedule comm task %s between %s -> %s. It costs %.f bytes",
              SD_task_get_name(after),
              SD_workstation_get_name(after->workstation_list[0]),SD_workstation_get_name(after->workstation_list[1]),
              after->communication_amount[2]);

        }
      }
    }
  }
}
/** @brief autoschedule a task on a list of workstations
 *
 * This function is very similar to SD_task_schedulev(),
 * but takes the list of workstations to schedule onto as separate parameters.
 * It builds a proper vector of workstations and then call SD_task_schedulev()
 */
void SD_task_schedulel(SD_task_t task, int count, ...) {
  va_list ap;
  SD_workstation_t *list=xbt_new(SD_workstation_t,count);
  int i;
  va_start(ap,count);
  for (i=0;i<count;i++) {
      list[i] = va_arg(ap,SD_workstation_t);
  }
  va_end(ap);
  SD_task_schedulev(task,count,list);
  free(list);
}
