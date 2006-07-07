#include "private.h"
#include "simdag/simdag.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_task,sd,
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
SD_task_t SD_task_create(const char *name, void *data, double amount) {
  SD_CHECK_INIT_DONE();

  SD_task_t task = xbt_new0(s_SD_task_t, 1);

  /* general information */
  task->data = data; /* user data */
  if (name != NULL)
    task->name = xbt_strdup(name);
  else
    task->name = NULL;

  task->state_set = sd_global->not_scheduled_task_set;
  xbt_swag_insert(task,task->state_set);

  task->amount = amount;
  task->surf_action = NULL;
  task->watch_points = 0;
  task->state_changed = 0;

  /* dependencies */
  task->tasks_before = xbt_dynar_new(sizeof(SD_dependency_t), NULL);
  task->tasks_after = xbt_dynar_new(sizeof(SD_dependency_t), NULL);

  /* scheduling parameters */
  task->workstation_nb = 0;
  task->workstation_list = NULL;
  task->computation_amount = NULL;
  task->communication_amount = NULL;
  task->rate = 0;

  return task;
}

/**
 * \brief Returns the user data of a task
 *
 * \param task a task
 * \return the user data associated with this task (can be \c NULL)
 * \see SD_task_set_data()
 */
void* SD_task_get_data(SD_task_t task) {
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
void SD_task_set_data(SD_task_t task, void *data) {
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
e_SD_task_state_t SD_task_get_state(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  if (task->state_set == sd_global->scheduled_task_set)
    return SD_SCHEDULED;
  if (task->state_set == sd_global->done_task_set)
    return SD_DONE;
  if (task->state_set == sd_global->running_task_set)
    return SD_RUNNING;
  if (task->state_set == sd_global->ready_task_set)
    return SD_READY;
  if (task->state_set == sd_global->not_scheduled_task_set)
    return SD_NOT_SCHEDULED;
  return SD_FAILED;
}

/* Changes the state of a task. Updates the swags and the flag sd_global->watch_point_reached.
 */
void __SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state) {
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
  case SD_RUNNING:
    task->state_set = sd_global->running_task_set;
    break;
  case SD_DONE:
    task->state_set = sd_global->done_task_set;
    break;
  case SD_FAILED:
    task->state_set = sd_global->failed_task_set;
    break;
  default:
    xbt_assert0(0, "Invalid state");
  }
  xbt_swag_insert(task, task->state_set);

  if (task->watch_points & new_state) {
    INFO1("Watch point reached with task '%s'!", SD_task_get_name(task));
    sd_global->watch_point_reached = 1;
    SD_task_unwatch(task, new_state); /* remove the watch point */
  }
}

/**
 * \brief Returns the name of a task
 *
 * \param task a task
 * \return the name of this task (can be \c NULL)
 */
const char* SD_task_get_name(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  return task->name;
}

/**
 * \brief Returns the total amount of a task
 *
 * \param task a task
 * \return the total amount of this task
 * \see SD_task_get_remaining_amount()
 */
double SD_task_get_amount(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  return task->amount;
}

/**
 * \brief Returns the remaining amount of a task
 *
 * \param task a task
 * \return the remaining amount of this task
 * \see SD_task_get_amount()
 */
double SD_task_get_remaining_amount(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  if (task->surf_action)
    return task->surf_action->remains;
  else
    return task->amount;
}

/* temporary function for debbuging */
static void __SD_print_dependencies(SD_task_t task) {
  INFO1("The following tasks must be executed before %s:", SD_task_get_name(task));
  xbt_dynar_t dynar = task->tasks_before;
  int length = xbt_dynar_length(dynar);
  int i;
  SD_dependency_t dependency;
  for (i = 0; i < length; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    INFO1(" %s", SD_task_get_name(dependency->src));
  }

  INFO1("The following tasks must be executed after %s:", SD_task_get_name(task));

  dynar = task->tasks_after;
  length = xbt_dynar_length(dynar);
  for (i = 0; i < length; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    INFO1(" %s", SD_task_get_name(dependency->dst));
  }
  INFO0("----------------------------");
}

/* Destroys a dependency between two tasks.
 */
static void __SD_task_dependency_destroy(void *dependency) {
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
void SD_task_dependency_add(const char *name, void *data, SD_task_t src, SD_task_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  xbt_assert1(src != dst, "Cannot add a dependency between task '%s' and itself", SD_task_get_name(src));
  xbt_assert1(__SD_task_is_not_scheduled(src) || __SD_task_is_scheduled_or_ready(src),
	      "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULED or SD_READY", SD_task_get_name(src));
  xbt_assert1(__SD_task_is_not_scheduled(dst) || __SD_task_is_scheduled_or_ready(dst),
	      "Task '%s' must be SD_NOT_SCHEDULED, SD_SCHEDULED or SD_READY", SD_task_get_name(dst));

  xbt_dynar_t dynar = src->tasks_after;
  int length = xbt_dynar_length(dynar);
  int found = 0;
  int i;
  SD_dependency_t dependency;
  for (i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    found = (dependency->dst == dst);
  }
  xbt_assert2(!found, "A dependency already exists between task '%s' and task '%s'",
	      SD_task_get_name(src), SD_task_get_name(dst));

  dependency = xbt_new0(s_SD_dependency_t, 1);

  if (name != NULL)
    dependency->name = xbt_strdup(name);
  dependency->data = data;
  dependency->src = src;
  dependency->dst = dst;

  /* src must be executed before dst */
  xbt_dynar_push(src->tasks_after, &dependency);
  xbt_dynar_push(dst->tasks_before, &dependency);

  /* if the task was ready, then dst->tasks_before is not empty anymore,
     so we must go back to state SD_SCHEDULED */
  if (__SD_task_is_ready(dst)) {
    DEBUG1("SD_task_dependency_add: %s was ready and becomes scheduled!", SD_task_get_name(dst));
    __SD_task_set_state(dst, SD_SCHEDULED);
  }

  /*  __SD_print_dependencies(src);
      __SD_print_dependencies(dst); */
}

/**
 * \brief Remove a dependency between two tasks
 *
 * \param src a task
 * \param dst a task depending on \a src
 * \see SD_task_dependency_add()
 */
void SD_task_dependency_remove(SD_task_t src, SD_task_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");

  /* remove the dependency from src->tasks_after */
  xbt_dynar_t dynar = src->tasks_after;
  int length = xbt_dynar_length(dynar);
  int found = 0;
  int i;
  SD_dependency_t dependency;
  for (i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    if (dependency->dst == dst) {
      xbt_dynar_remove_at(dynar, i, NULL);
      found = 1;
    }
  }
  xbt_assert4(found, "No dependency found between task '%s' and '%s': task '%s' is not a successor of task '%s'",
	      SD_task_get_name(src), SD_task_get_name(dst), SD_task_get_name(dst), SD_task_get_name(src));

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
  xbt_assert4(found, "SimDag error: task '%s' is a successor of '%s' but task '%s' is not a predecessor of task '%s'",
	      SD_task_get_name(dst), SD_task_get_name(src), SD_task_get_name(src), SD_task_get_name(dst)); /* should never happen... */

  /* if the task was scheduled and dst->tasks_before is empty now, we can make it ready */
  if (xbt_dynar_length(dst->tasks_before) == 0 && __SD_task_is_scheduled(dst))
    __SD_task_set_state(dst, SD_READY);

  /*  __SD_print_dependencies(src); 
      __SD_print_dependencies(dst);*/
}

/**
 * \brief Returns the user data associated with a dependency between two tasks
 *
 * \param src a task
 * \param dst a task depending on \a src
 * \return the user data associated with this dependency (can be \c NULL)
 * \see SD_task_dependency_add()
 */
void *SD_task_dependency_get_data(SD_task_t src, SD_task_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");

  xbt_dynar_t dynar = src->tasks_after;
  int length = xbt_dynar_length(dynar);
  int found = 0;
  int i;
  SD_dependency_t dependency;
  for (i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    found = (dependency->dst == dst);
  }
  xbt_assert2(found, "No dependency found between task '%s' and '%s'", SD_task_get_name(src), SD_task_get_name(dst));
  return dependency->data;
}

/* temporary function for debugging */
static void __SD_print_watch_points(SD_task_t task) {
  static const int state_masks[] = {SD_SCHEDULED, SD_RUNNING, SD_READY, SD_DONE, SD_FAILED};
  static const char* state_names[] = {"scheduled", "running", "ready", "done", "failed"};

  INFO2("Task '%s' watch points (%x): ", SD_task_get_name(task), task->watch_points);

  int i;
  for (i = 0; i < 5; i++) {
    if (task->watch_points & state_masks[i])
      INFO1("%s ", state_names[i]);
  }
}

/**
 * \brief Adds a watch point to a task
 *
 * SD_simulate() will stop as soon as the \ref e_SD_task_state_t "state" of this task becomes the one given in argument. The
 * watch point is then automatically removed.
 * 
 * \param task a task
 * \param state the \ref e_SD_task_state_t "state" you want to watch (cannot be #SD_NOT_SCHEDULED)
 * \see SD_task_unwatch()
 */
void SD_task_watch(SD_task_t task, e_SD_task_state_t state) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  xbt_assert0(state != SD_NOT_SCHEDULED, "Cannot add a watch point for state SD_NOT_SCHEDULED");

  task->watch_points = task->watch_points | state;
  /*  __SD_print_watch_points(task);*/
}

/**
 * \brief Removes a watch point from a task
 * 
 * \param task a task
 * \param state the \ref e_SD_task_state_t "state" you no longer want to watch
 * \see SD_task_watch()
 */
void SD_task_unwatch(SD_task_t task, e_SD_task_state_t state) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  xbt_assert0(state != SD_NOT_SCHEDULED, "Cannot have a watch point for state SD_NOT_SCHEDULED");
  
  task->watch_points = task->watch_points & ~state;
  /*  __SD_print_watch_points(task);*/
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
 * \param rate task execution speed rate
 * \see SD_schedule()
 */
double SD_task_get_execution_time(SD_task_t task,
				  int workstation_nb,
				  const SD_workstation_t *workstation_list,
				  const double *computation_amount,
				  const double *communication_amount,
				  double rate) {
  /* the task execution time is the maximum execution time of the parallel tasks */
  double time, max_time = 0.0;
  int i, j;
  for (i = 0; i < workstation_nb; i++) {
    time = SD_workstation_get_computation_time(workstation_list[i], computation_amount[i]);
    
    for (j = 0; j < workstation_nb; j++) {
      time += SD_route_get_communication_time(workstation_list[i], workstation_list[j],
					      communication_amount[i * workstation_nb + j]);
    }

    if (time > max_time) {
      max_time = time;
    }
  }
  return max_time * SD_task_get_amount(task);
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
void SD_task_schedule(SD_task_t task, int workstation_nb,
		     const SD_workstation_t *workstation_list, const double *computation_amount,
		     const double *communication_amount, double rate) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  xbt_assert1(__SD_task_is_not_scheduled(task), "Task '%s' has already been scheduled.", SD_task_get_name(task));
  xbt_assert0(workstation_nb > 0, "workstation_nb must be positive");

  task->workstation_nb = workstation_nb;
  task->rate = rate;

  task->computation_amount = xbt_new0(double, workstation_nb);
  memcpy(task->computation_amount, computation_amount, sizeof(double) * workstation_nb);

  int communication_nb = workstation_nb * workstation_nb;
  task->communication_amount = xbt_new0(double, communication_nb);
  memcpy(task->communication_amount, communication_amount, sizeof(double) * communication_nb);

  /* we have to create a Surf workstation array instead of the SimDag workstation array */
  task->workstation_list = xbt_new0(void*, workstation_nb);
  int i;
  for (i = 0; i < workstation_nb; i++) {
    task->workstation_list[i] = workstation_list[i]->surf_workstation;
  }

  /* update the task state */
  if (xbt_dynar_length(task->tasks_before) == 0)
    __SD_task_set_state(task, SD_READY);
  else
    __SD_task_set_state(task, SD_SCHEDULED);
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
void SD_task_unschedule(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  xbt_assert1(task->state_set == sd_global->scheduled_task_set ||
	      task->state_set == sd_global->ready_task_set ||
	      task->state_set == sd_global->running_task_set ||
	      task->state_set == sd_global->failed_task_set,
	      "Task %s: the state must be SD_SCHEDULED, SD_READY, SD_RUNNING or SD_FAILED",
	      SD_task_get_name(task));

  if (__SD_task_is_scheduled_or_ready(task)) /* if the task is scheduled or ready */
    __SD_task_destroy_scheduling_data(task);

  if (__SD_task_is_running(task)) /* the task should become SD_FAILED */
    surf_workstation_resource->common_public->action_cancel(task->surf_action);
  else
    __SD_task_set_state(task, SD_NOT_SCHEDULED);
}

/* Destroys the data memorised by SD_task_schedule. Task state must be SD_SCHEDULED or SD_READY.
 */
static void __SD_task_destroy_scheduling_data(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert1(__SD_task_is_scheduled_or_ready(task),
	      "Task '%s' must be SD_SCHEDULED or SD_READY", SD_task_get_name(task));
  xbt_free(task->workstation_list);
  xbt_free(task->computation_amount);
  xbt_free(task->communication_amount);
}

/* Runs a task. This function is called by SD_simulate() when a scheduled task can start
 * (ie when its dependencies are satisfied).
 */
surf_action_t __SD_task_run(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  xbt_assert2(__SD_task_is_ready(task), "Task '%s' is not ready! Task state: %d",
	      SD_task_get_name(task), SD_task_get_state(task));

  surf_action_t surf_action = surf_workstation_resource->extension_public->
    execute_parallel_task(task->workstation_nb,
			  task->workstation_list,
			  task->computation_amount,
			  task->communication_amount,
			  task->amount,
			  task->rate);

  DEBUG1("surf_action = %p", surf_action);

  __SD_task_destroy_scheduling_data(task); /* now the scheduling data are not useful anymore */
  __SD_task_set_state(task, SD_RUNNING);

  return surf_action;
}
/* Remove all dependencies associated with a task. This function is called when the task is destroyed.
 */
static void __SD_task_remove_dependencies(SD_task_t task) {
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
 * \brief Destroys a task.
 *
 * The user data (if any) should have been destroyed first.
 *
 * \param task the task you want to destroy
 * \see SD_task_create()
 */
void SD_task_destroy(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  DEBUG1("Destroying task %s...", SD_task_get_name(task));

  __SD_task_remove_dependencies(task);

  /* if the task was scheduled or ready we have to free the scheduling parameters */
  if (__SD_task_is_scheduled_or_ready(task))
    __SD_task_destroy_scheduling_data(task);

  if (task->name != NULL)
    xbt_free(task->name);

  if (task->surf_action != NULL)
    surf_workstation_resource->common_public->action_free(task->surf_action);

  xbt_dynar_free(&task->tasks_before);
  xbt_dynar_free(&task->tasks_after);
  xbt_free(task);

  DEBUG0("Task destroyed.");
}
