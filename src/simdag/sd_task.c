#include "simdag/simdag.h"
#include "xbt/sysdep.h"

/* Creates a task.
 */
SD_task_t SD_task_create(const char *name, void *data, double amount) {

  xbt_assert0(amount >= 0, "Invalid parameter"); /* or amount > 0 ? */

  SD_task_t task = xbt_new0(s_SD_task_t, 1);
  
  task->data = data;
  task->name = xbt_strdup(name);
  /*task->amount = amount;
    task->remaining_amount = amount;*/
  task->state = SD_SCHEDULED; /* not sure... should we add a state SD_NOT_SCHEDULED? */
  /* TODO: dependencies + watch */

  return task;
}

/* Schedules a task.
 */
int SD_task_schedule(SD_task_t task, int workstation_nb,
		     SD_workstation_t **workstation_list, double *computation_amount,
		     double *communication_amount, double rate) {
  xbt_assert0(task, "Invalid parameter");
  /* TODO */

  return 0;
}

/* Returns the data of a task.
 */
void* SD_task_get_data(SD_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  return task->data;
}

/* Sets the data of a task.
 */
void SD_task_set_data(SD_task_t task, void *data) {
  xbt_assert0(task, "Invalid parameter");
  task->data = data;
}

/* Returns the name of a task.
 */
const char* SD_task_get_name(SD_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  return task->name;
}

/* Returns the computing amount of a task.
 */
double SD_task_get_amount(SD_task_t task) {
  xbt_assert0(task, "Invalid parameter");

  /* TODO */
  return 0;
  /*return task->amount;*/
}

/* Returns the remaining computing amount of a task.
 */
double SD_task_get_remaining_amount(SD_task_t task) {
  xbt_assert0(task, "Invalid parameter")

  /* TODO (surf encapsulation) */;
  return 0;
}

/* Adds a dependency between two tasks.
 */
void SD_task_dependency_add(const char *name, void *data, SD_task_t src, SD_task_t dst) {
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  /* TODO */
}

/* Removes a dependency between two tasks.
 */
void SD_task_dependency_remove(SD_task_t src, SD_task_t dst) {
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  /* TODO */
}

/* Returns the state of a task: SD_SCHEDULED, SD_RUNNING, SD_DONE or SD_FAILED.
 */
SD_task_state_t SD_task_get_state(SD_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  return task->state;
}

/* Adds a watch point to a task.
   SD_simulate will stop as soon as the state of this task is the one given in argument.
   Watch point is then automatically removed.
 */
void  SD_task_watch(SD_task_t task, SD_task_state_t state) {
  xbt_assert0(task, "Invalid parameter");
  /* TODO */
}

/* Removes a watch point from a task.
 */
void SD_task_unwatch(SD_task_t task, SD_task_state_t state) {
  xbt_assert0(task, "Invalid parameter");
  /* TODO */
}

/* Unschedules a task.
   Change state and rerun
 */
void SD_task_unschedule(SD_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  /* TODO */
}

/* Destroys a task. The user data (if any) should have been destroyed first.
 */
void SD_task_destroy(SD_task_t task) {
  if (task->name)
    xbt_free(task->name);

  /* TODO: dependencies + watch */
  xbt_free(task);
}
