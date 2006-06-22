#ifndef SIMDAG_PRIVATE_H
#define SIMDAG_PRIVATE_H

#include "xbt/dict.h"
#include "xbt/dynar.h"
#include "simdag/simdag.h"
#include "simdag/datatypes.h"
#include "surf/surf.h"

#define SD_CHECK_INIT_DONE() xbt_assert0(sd_global != NULL, "Call SD_init() first")

/* Global variables */

typedef struct SD_global {
  xbt_dict_t workstations; /* workstation list */
  int workstation_count; /* number of workstations */
  xbt_dict_t links; /* link list */
} s_SD_global_t, *SD_global_t;

extern SD_global_t sd_global;

/* Link private data */
typedef struct SD_link_data {
  void *surf_link; /* surf object */
} s_SD_link_data_t;

/* Workstation private data */
typedef struct SD_workstation_data {
  void *surf_workstation; /* surf object */
} s_SD_workstation_data_t;

/* Task dependencies */
typedef struct SD_dependency {
  char *name;
  void *data;
  SD_task_t src;
  SD_task_t dst;
  /* src must be finished before dst can start */
} s_SD_dependency_t, *SD_dependency_t;

/* Task private data */
typedef struct SD_task_data {
  char *name;
  SD_task_state_t state;
  double amount;
  surf_action_t surf_action;
  unsigned short watch_points;

  /* dependencies */
  xbt_dynar_t tasks_before;
  xbt_dynar_t tasks_after;

  /* scheduling parameters (only exist in state SD_SCHEDULED) */
  int workstation_nb;
  void **workstation_list; /* surf workstations */
  double *computation_amount;
  double *communication_amount;
  double rate;
} s_SD_task_data_t;

/* Private functions */

SD_link_t __SD_link_create(void *surf_link, void *data);
void __SD_link_destroy(void *link);

SD_workstation_t __SD_workstation_create(void *surf_workstation, void *data);
void __SD_workstation_destroy(void *workstation);

void __SD_task_run(SD_task_t task);
void __SD_task_destroy(SD_task_t task);
void __SD_task_destroy_scheduling_data(SD_task_t task);
void __SD_task_destroy_dependency(void *dependency);

#endif
