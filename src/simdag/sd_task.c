#include "private.h"
#include "simdag/simdag.h"
#include "xbt/sysdep.h"

/* Creates a task.
 */
SD_task_t SD_task_create(const char *name, void *data, double amount) {
  CHECK_INIT_DONE();
  xbt_assert0(name != NULL, "name is NULL !");
  xbt_assert0(amount >= 0, "amount must be positive"); /* or amount > 0 ? */

  SD_task_data_t sd_data = xbt_new0(s_SD_task_data_t, 1); /* task private data */
  sd_data->name = xbt_strdup(name);
  sd_data->state = SD_SCHEDULED; /* not sure */

  SD_task_t task = xbt_new0(s_SD_task_t, 1);
  task->sd_data = sd_data; /* private data */
  task->data = data; /* user data */

  /* TODO: dependencies + watch */

  return task;
}

/* Schedules a task.
 */
int SD_task_schedule(SD_task_t task, int workstation_nb,
		     SD_workstation_t **workstation_list, double *computation_amount,
		     double *communication_amount, double rate) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  /* TODO */

  return 0;
}

/* Returns the data of a task.
 */
void* SD_task_get_data(SD_task_t task) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  return task->data;
}

/* Sets the data of a task.
 */
void SD_task_set_data(SD_task_t task, void *data) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  task->data = data;
}

/* Returns the name of a task.
 */
const char* SD_task_get_name(SD_task_t task) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  return task->sd_data->name;
}

/* Returns the computing amount of a task.
 */
double SD_task_get_amount(SD_task_t task) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");

  /* TODO */
  return 0;
  /*return task->amount;*/
}

/* Returns the remaining computing amount of a task.
 */
double SD_task_get_remaining_amount(SD_task_t task) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter")

  /* TODO (surf encapsulation) */;
  return 0;
}

/* Adds a dependency between two tasks.
 */
void SD_task_dependency_add(const char *name, void *data, SD_task_t src, SD_task_t dst) {
  CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  /* TODO */
}

/* Removes a dependency between two tasks.
 */
void SD_task_dependency_remove(SD_task_t src, SD_task_t dst) {
  CHECK_INIT_DONE();
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  /* TODO */
}

/* Returns the state of a task: SD_SCHEDULED, SD_RUNNING, SD_DONE or SD_FAILED.
 */
SD_task_state_t SD_task_get_state(SD_task_t task) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  return task->sd_data->state;
}

/* Adds a watch point to a task.
   SD_simulate will stop as soon as the state of this task is the one given in argument.
   Watch point is then automatically removed.
 */
void  SD_task_watch(SD_task_t task, SD_task_state_t state) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  /* TODO */
}

/* Removes a watch point from a task.
 */
void SD_task_unwatch(SD_task_t task, SD_task_state_t state) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  /* TODO */
}

/* Unschedules a task.
   Change state and rerun
 */
void SD_task_unschedule(SD_task_t task) {
  CHECK_INIT_DONE();
  xbt_assert0(task, "Invalid parameter");
  /* TODO */
}

/* Destroys a task. The user data (if any) should have been destroyed first.
 */
void SD_task_destroy(SD_task_t task) {
  CHECK_INIT_DONE();
  xbt_free(task->sd_data->name);

  /* TODO: dependencies + watch */
  xbt_free(task);
}
