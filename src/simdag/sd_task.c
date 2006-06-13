#include "simdag/simdag.h"
#include "xbt/sysdep.h"

/* Creates a task.
 */
SG_task_t SG_task_create(const char *name, void *data, double amount) {

  xbt_assert0(amount >= 0, "Invalid parameter"); /* or amount > 0 ? */

  SG_task_t task = xbt_new0(s_SG_task_t, 1);
  
  task->data = data;
  task->name = xbt_strdup(name);
  task->amount = amount;
  task->remaining_amount = amount;
  task->state = SG_SCHEDULED; /* not sure... should we add a state SG_NOT_SCHEDULED? */
  /* TODO: dependencies + watch */

  return task;
}

/* Schedules a task.
 */
int SG_task_schedule(SG_task_t task, int workstation_nb,
		     SG_workstation_t **workstation_list, double *computation_amount,
		     double *communication_amount, double rate) {
  xbt_assert0(task, "Invalid parameter");
  /* TODO */

  return 0;
}

/* Returns the data of a task.
 */
void* SG_task_get_data(SG_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  return task->data;
}

/* Sets the data of a task.
 */
void SG_task_set_data(SG_task_t task, void *data) {
  xbt_assert0(task, "Invalid parameter");
  task->data = data;
}

/* Returns the name of a task.
 */
const char* SG_task_get_name(SG_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  return task->name;
}

/* Returns the computing amount of a task.
 */
double SG_task_get_amount(SG_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  return task->amount;
}

/* Returns the remaining computing amount of a task.
 */
double SG_task_get_remaining_amount(SG_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  return task->remaining_amount;
}

/* Adds a dependency between two tasks.
 */
void SG_task_dependency_add(const char *name, void *data, SG_task_t src, SG_task_t dst) {
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  /* TODO */
}

/* Removes a dependency between two tasks.
 */
void SG_task_dependency_remove(SG_task_t src, SG_task_t dst) {
  xbt_assert0(src != NULL && dst != NULL, "Invalid parameter");
  /* TODO */
}

/* Returns the state of a task: SG_SCHEDULED, SG_RUNNING, SG_DONE or SG_FAILED.
 */
SG_task_state_t SG_task_get_state(SG_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  return task->state;
}

/* Adds a watch point to a task.
   SG_simulate will stop as soon as the state of this task is the one given in argument.
   Watch point is then automatically removed.
 */
void  SG_task_watch(SG_task_t task, SG_task_state_t state) {
  xbt_assert0(task, "Invalid parameter");
  /* TODO */
}

/* Removes a watch point from a task.
 */
void SG_task_unwatch(SG_task_t task, SG_task_state_t state) {
  xbt_assert0(task, "Invalid parameter");
  /* TODO */
}

/* Unschedules a task.
   Change state and rerun
 */
void SG_task_unschedule(SG_task_t task) {
  xbt_assert0(task, "Invalid parameter");
  /* TODO */
}

/* Destroys a task. The user data (if any) should have been destroyed first.
 */
void SG_task_destroy(SG_task_t task) {
  if (task->name)
    free(task->name);

  /* TODO: dependencies + watch */
  free(task);
}
