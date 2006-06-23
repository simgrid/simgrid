#include "private.h"
#include "simdag/simdag.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"

static void __SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state);

/* Creates a task.
 */
SD_task_t SD_task_create(const char *name, void *data, double amount) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(amount > 0, "amount must be positive");

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

  /* dependencies */
  task->tasks_before = xbt_dynar_new(sizeof(SD_dependency_t), NULL);
  task->tasks_after = xbt_dynar_new(sizeof(SD_dependency_t), NULL);

  /* scheduling parameters */
  task->workstation_nb = 0;
  task->workstation_list = NULL;
  task->computation_amount = NULL;
  task->communication_amount = NULL;
  task->rate = 0;

  xbt_dynar_push(sd_global->tasks, &task);

  return task;
}

/* Returns the data of a task.
 */
void* SD_task_get_data(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  return task->data;
}

/* Returns the state of a task: SD_NOT_SCHEDULED, SD_SCHEDULED, SD_RUNNING, SD_DONE or SD_FAILED.
 */
e_SD_task_state_t SD_task_get_state(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  if (task->state_set == sd_global->not_scheduled_task_set)
    return SD_NOT_SCHEDULED;
  if (task->state_set == sd_global->scheduled_task_set)
    return SD_SCHEDULED;
  if (task->state_set == sd_global->running_task_set)
    return SD_RUNNING;
  if (task->state_set == sd_global->done_task_set)
    return SD_DONE;
  return SD_FAILED;
}

/* Changes the state of a task. Update the swags and the flag sd_global->watch_point_reached.
 */
static void __SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state) {
  xbt_swag_remove(task, task->state_set);
  switch (new_state) {
  case SD_NOT_SCHEDULED:
    task->state_set = sd_global->not_scheduled_task_set;
    break;
  case SD_SCHEDULED:
    task->state_set = sd_global->scheduled_task_set;
    break;
  case SD_RUNNING:
    task->state_set = sd_global->running_task_set;
    break;
  case SD_DONE:
    task->state_set = sd_global->done_task_set;
    break;
  default: /* SD_FAILED */
    task->state_set = sd_global->failed_task_set;
  }
  xbt_swag_insert(task,task->state_set);

  if (task->watch_points & new_state) {
    printf("Watch point reached with task '%s' in state %d!\n", SD_task_get_name(task), new_state);
    sd_global->watch_point_reached = 1;
  }
}

/* Sets the data of a task.
 */
void SD_task_set_data(SD_task_t task, void *data) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  task->data = data;
}

/* Returns the name of a task. The name can be NULL.
 */
const char* SD_task_get_name(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  return task->name;
}

/* Returns the computing amount of a task.
 */
double SD_task_get_amount(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  return task->amount;
}

/* Returns the remaining computing amount of a task.
 */
double SD_task_get_remaining_amount(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  
  if (task->surf_action)
    return task->amount;
  else
    return task->surf_action->remains;
}

/* temporary function for debbuging
void __SD_print_dependencies(SD_task_t task) {
  printf("The following tasks must be executed before %s:", SD_task_get_name(task));
  xbt_dynar_t dynar = task->tasks_before;
  int length = xbt_dynar_length(dynar);
  int i;
  SD_dependency_t dependency;
  for (i = 0; i < length; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    printf(" %s", SD_task_get_name(dependency->src));
  }

  printf("\nThe following tasks must be executed after %s:", SD_task_get_name(task));

  dynar = task->tasks_after;
  length = xbt_dynar_length(dynar);
  for (i = 0; i < length; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    printf(" %s", SD_task_get_name(dependency->dst));
  }
  printf("\n----------------------------\n");
}*/

/* Destroys a dependency between two tasks.
 */
void __SD_task_destroy_dependency(void *dependency) {
  if (((SD_dependency_t) dependency)->name != NULL)
    xbt_free(((SD_dependency_t) dependency)->name);
  xbt_free(dependency);
}

/* Adds a dependency between two tasks.
 */
void SD_task_dependency_add(const char *name, void *data, SD_task_t src, SD_task_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  xbt_assert1(src != dst, "Cannot add a dependency between task '%s' and itself", SD_task_get_name(src));

  xbt_dynar_t dynar = src->tasks_after;
  int length = xbt_dynar_length(dynar);
  int found = 0;
  int i;
  SD_dependency_t dependency;
  for (i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    found = (dependency->dst == dst);
  }
  xbt_assert2(!found, "A dependency already exists between task '%s' and task '%s'", SD_task_get_name(src), SD_task_get_name(dst));

  dependency = xbt_new0(s_SD_dependency_t, 1);

  if (name != NULL)
    dependency->name = xbt_strdup(name);
  dependency->data = data;
  dependency->src = src;
  dependency->dst = dst;

  /* src must be executed before dst */
  xbt_dynar_push(src->tasks_after, &dependency);
  xbt_dynar_push(dst->tasks_before, &dependency);

  /*  __SD_print_dependencies(src);
      __SD_print_dependencies(dst); */
}

/* Removes a dependency between two tasks.
 */
void SD_task_dependency_remove(SD_task_t src, SD_task_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  xbt_assert1(src != dst, "Cannot remove a dependency between task '%s' and itself", SD_task_get_name(src));

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

  dynar = dst->tasks_before;
  length = xbt_dynar_length(dynar);
  found = 0;
  
  for (i = 0; i < length && !found; i++) {
    xbt_dynar_get_cpy(dynar, i, &dependency);
    if (dependency->src == src) {
      xbt_dynar_remove_at(dynar, i, NULL);
      __SD_task_destroy_dependency(dependency);
      found = 1;
    }
  }
  xbt_assert4(found, "SimDag error: task '%s' is a successor of '%s' but task '%s' is not a predecessor of task '%s'",
	      SD_task_get_name(dst), SD_task_get_name(src), SD_task_get_name(src), SD_task_get_name(dst)); /* should never happen... */

  /*   __SD_print_dependencies(src); 
       __SD_print_dependencies(dst); */
}

/* Returns the data associated to a dependency between two tasks. This data can be NULL.
 */
void *SD_task_dependency_get_data(SD_task_t src, SD_task_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  xbt_assert1(src != dst, "Cannot have a dependency between task '%s' and itself", SD_task_get_name(src));

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
  static const int state_masks[] = {SD_SCHEDULED, SD_RUNNING, SD_DONE, SD_FAILED};
  static const char* state_names[] = {"scheduled", "running", "done", "failed"};

  printf("Task '%s' watch points (%x): ", SD_task_get_name(task), task->watch_points);

  int i;
  for (i = 0; i < 4; i++) {
    if (task->watch_points & state_masks[i])
      printf("%s ", state_names[i]);
  }
  printf("\n");
}

/* Adds a watch point to a task.
   SD_simulate will stop as soon as the state of this task is the one given in argument.
   Watch point is then automatically removed.
 */
void SD_task_watch(SD_task_t task, e_SD_task_state_t state) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  task->watch_points = task->watch_points | state;
  __SD_print_watch_points(task);
}

/* Removes a watch point from a task.
 */
void SD_task_unwatch(SD_task_t task, e_SD_task_state_t state) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  
  task->watch_points = task->watch_points & ~state;
  __SD_print_watch_points(task);
}

/* Destroys the data memorised by SD_task_schedule. Task state must be SD_SCHEDULED.
 */
static void __SD_task_destroy_scheduling_data(SD_task_t task) {
  xbt_free(task->workstation_list);
  xbt_free(task->computation_amount);
  xbt_free(task->communication_amount);
}

/* Schedules a task.
 * task: the task to schedule
 * workstation_nb: number of workstations where the task will be executed
 * workstation_list: workstations where the task will be executed
 * computation_amount: computation amount for each workstation
 * communication_amount: communication amount between each pair of workstations
 * rate: task execution speed rate
 */
void SD_task_schedule(SD_task_t task, int workstation_nb,
		     const SD_workstation_t *workstation_list, double *computation_amount,
		     double *communication_amount, double rate) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  xbt_assert1(SD_task_get_state(task) == SD_NOT_SCHEDULED, "Task '%s' has already been scheduled.", SD_task_get_name(task));
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

  __SD_task_set_state(task, SD_SCHEDULED);
}

/* Unschedules a task. The state must be SD_SCHEDULED, SD_RUNNING or SD_FAILED.
 * The task is reinitialised and its state becomes SD_NOT_SCHEDULED.
 * Call SD_task_schedule to schedule it again.
 */
void SD_task_unschedule(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");
  e_SD_task_state_t state = SD_task_get_state(task);
  xbt_assert1(state == SD_SCHEDULED ||
	      state == SD_RUNNING ||
	      state == SD_FAILED,
	      "Task %s: the state must be SD_SCHEDULED, SD_RUNNING or SD_FAILED", SD_task_get_name(task));

  if (state == SD_SCHEDULED)
    __SD_task_destroy_scheduling_data(task);

  __SD_task_set_state(task, SD_NOT_SCHEDULED);
}

/* Runs a task. This function is called by SD_simulate when a scheduled task can start
 * (ie when its dependencies are satisfied).
 */
surf_action_t __SD_task_run(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");


  surf_action_t surf_action = surf_workstation_resource->extension_public->
    execute_parallel_task(task->workstation_nb,
			  task->workstation_list,
			  task->computation_amount,
			  task->communication_amount,
			  task->amount,
			  task->rate);
  __SD_task_set_state(task, SD_RUNNING);

  __SD_task_destroy_scheduling_data(task); /* now the scheduling data are not useful anymore */
  return surf_action;
}


/* Destroys a task. The user data (if any) should have been destroyed first.
 */
void SD_task_destroy(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task != NULL, "Invalid parameter");

  /*printf("Destroying task %s...\n", SD_task_get_name(task));*/

  /* remove the task from SimDag task list */
  int index = xbt_dynar_search(sd_global->tasks, &task);
  xbt_dynar_remove_at(sd_global->tasks, index, NULL);

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

  /* if the task was scheduled we have to free the scheduling parameters */
  if (SD_task_get_state(task) == SD_SCHEDULED)
    __SD_task_destroy_scheduling_data(task);

  if (task->name != NULL)
    xbt_free(task->name);

  xbt_dynar_free(&task->tasks_before);
  xbt_dynar_free(&task->tasks_after);
  xbt_free(task);

  /*printf("Task destroyed.\n");*/

}
