#ifndef SIMDAG_PRIVATE_H
#define SIMDAG_PRIVATE_H

#include "xbt/dict.h"
#include "simdag/simdag.h"
#include "simdag/datatypes.h"
#include "surf/surf.h"

#define CHECK_INIT_DONE() xbt_assert0(sd_global != NULL, "SD_init not called yet")

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

/* Task private data */
typedef struct SD_task_data {
  char *name;
  SD_task_state_t state;
  double amount;
  surf_action_t surf_action;
  unsigned short watch_points;

  /*  double remaining_amount;*/
  /* TODO: dependencies */
} s_SD_task_data_t;

/* Private functions */

SD_link_t __SD_link_create(void *surf_link, void *data);
void __SD_link_destroy(void *link);

SD_workstation_t __SD_workstation_create(void *surf_workstation, void *data);
void __SD_workstation_destroy(void *workstation);

#endif
