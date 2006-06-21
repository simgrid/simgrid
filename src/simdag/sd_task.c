#include "private.h"
#include "simdag/simdag.h"
#include "xbt/sysdep.h"

/* Creates a task.
 */
SD_task_t SD_task_create(const char *name, void *data, double amount) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(amount > 0, "amount must be positive");

  SD_task_data_t sd_data = xbt_new0(s_SD_task_data_t, 1); /* task private data */

  /* general information */
  if (name != NULL)
    sd_data->name = xbt_strdup(name);
  else
    sd_data->name = NULL;

  sd_data->state = SD_NOT_SCHEDULED;
  sd_data->amount = amount;
  sd_data->surf_action = NULL;
  sd_data->watch_points = 0;

  /* dependencies */
  sd_data->tasks_before = xbt_dynar_new(sizeof(SD_task_t), NULL);
  sd_data->tasks_after = xbt_dynar_new(sizeof(SD_task_t), NULL);

  /* scheduling parameters */
  sd_data->workstation_nb = 0;
  sd_data->workstation_list = NULL;
  sd_data->computation_amount = NULL;
  sd_data->communication_amount = NULL;
  sd_data->rate = 0;

  SD_task_t task = xbt_new0(s_SD_task_t, 1);
  task->sd_data = sd_data; /* private data */
  task->data = data; /* user data */

  return task;
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
  xbt_assert0(SD_task_get_state(task) == SD_NOT_SCHEDULED, "This task has already been scheduled.");
  xbt_assert0(workstation_nb > 0, "workstation_nb must be positive");

  SD_task_data_t sd_data = task->sd_data;
  sd_data->workstation_nb = workstation_nb;
  sd_data->rate = rate;

  sd_data->computation_amount = xbt_new0(double, workstation_nb);
  memcpy(sd_data->computation_amount, computation_amount, sizeof(double) * workstation_nb);

  int communication_nb = workstation_nb * workstation_nb;
  sd_data->communication_amount = xbt_new0(double, communication_nb);
  memcpy(sd_data->communication_amount, communication_amount, sizeof(double) * communication_nb);

  /* we have to create a Surf workstation array instead of the SimDag workstation array */
  sd_data->workstation_list = xbt_new0(void*, workstation_nb);
  int i;
  for (i = 0; i < workstation_nb; i++) {
    sd_data->workstation_list[i] = workstation_list[i]->sd_data->surf_workstation;
  }

  sd_data->state = SD_SCHEDULED;
}

/* Returns the data of a task.
 */
void* SD_task_get_data(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  return task->data;
}

/* Sets the data of a task.
 */
void SD_task_set_data(SD_task_t task, void *data) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  task->data = data;
}

/* Returns the name of a task. The name can be NULL.
 */
const char* SD_task_get_name(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  return task->sd_data->name;
}

/* Returns the computing amount of a task.
 */
double SD_task_get_amount(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  return task->sd_data->amount;
}

/* Returns the remaining computing amount of a task.
 */
double SD_task_get_remaining_amount(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  SD_task_data_t sd_data = task->sd_data;
  if (sd_data->surf_action)
    return sd_data->amount;
  else
    return sd_data->surf_action->remains;
}

/* Adds a dependency between two tasks.
 */
void SD_task_dependency_add(const char *name, void *data, SD_task_t src, SD_task_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  /* TODO */
}

/* Removes a dependency between two tasks.
 */
void SD_task_dependency_remove(SD_task_t src, SD_task_t dst) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  /* TODO */
}

/* Returns the state of a task: SD_NOT_SCHEDULED, SD_SCHEDULED, SD_RUNNING, SD_DONE or SD_FAILED.
 */
SD_task_state_t SD_task_get_state(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  return task->sd_data->state;
}

/* temporary function for debugging */
void __SD_print_watch_points(SD_task_t task) {
  static const int state_masks[] = {SD_SCHEDULED, SD_RUNNING, SD_DONE, SD_FAILED};
  static const char* state_names[] = {"scheduled", "running", "done", "failed"};

  printf("Task '%s' watch points (%x): ", task->sd_data->name, task->sd_data->watch_points);

  int i;
  for (i = 0; i < 4; i++) {
    if (task->sd_data->watch_points & state_masks[i])
      printf("%s ", state_names[i]);
  }
  printf("\n");
}

/* Adds a watch point to a task.
   SD_simulate will stop as soon as the state of this task is the one given in argument.
   Watch point is then automatically removed.
 */
void SD_task_watch(SD_task_t task, SD_task_state_t state) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");

  task->sd_data->watch_points = task->sd_data->watch_points | state;
  __SD_print_watch_points(task);
}

/* Removes a watch point from a task.
 */
void SD_task_unwatch(SD_task_t task, SD_task_state_t state) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  
  task->sd_data->watch_points = task->sd_data->watch_points & ~state;
  __SD_print_watch_points(task);
}

/* Unschedules a task. The state must be SD_SCHEDULED, SD_RUNNING or SD_FAILED.
 * The task is reinitialised and its state becomes SD_NOT_SCHEDULED.
 * Call SD_task_schedule to schedule it again.
 */
void SD_task_unschedule(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  xbt_assert1(task->sd_data->state == SD_SCHEDULED ||
	      task->sd_data->state == SD_RUNNING ||
	      task->sd_data->state == SD_FAILED,
	      "Task %s: the state must be SD_SCHEDULED, SD_RUNNING or SD_FAILED", task->sd_data->name);

  if (task->sd_data->state == SD_SCHEDULED)
    __SD_task_destroy_scheduling_data(task);

  task->sd_data->state = SD_NOT_SCHEDULED;
}

/* Runs a task. This function is called by SD_simulate when a scheduled task can start
 * (ie when its dependencies are satisfied).
 */
void __SD_task_run(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");

  SD_task_data_t sd_data = task->sd_data;
  surf_workstation_resource->extension_public->
    execute_parallel_task(sd_data->workstation_nb,
			  sd_data->workstation_list,
			  sd_data->computation_amount,
			  sd_data->communication_amount,
			  sd_data->amount,
			  sd_data->rate);
  task->sd_data->state = SD_RUNNING;

  __SD_task_destroy_scheduling_data(task); /* now the scheduling data are not useful anymore */
}


/* Destroys a task. The user data (if any) should have been destroyed first.
 */
void SD_task_destroy(SD_task_t task) {
  SD_CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");

  if (task->sd_data->state == SD_SCHEDULED)
    __SD_task_destroy_scheduling_data(task);

  if (task->sd_data->name != NULL)
    xbt_free(task->sd_data->name);

  xbt_dynar_free(&task->sd_data->tasks_before);
  xbt_dynar_free(&task->sd_data->tasks_after);
  xbt_free(task->sd_data);
  xbt_free(task);
}

/* Destroys the data memorised by SD_task_schedule. Task state must be SD_SCHEDULED.
 */
void __SD_task_destroy_scheduling_data(SD_task_t task) {
  xbt_free(task->sd_data->workstation_list);
  xbt_free(task->sd_data->computation_amount);
  xbt_free(task->sd_data->communication_amount);
}
