#ifndef SIMDAG_DATATYPES_H
#define SIMDAG_DATATYPES_H

/* Link */
typedef struct SD_link_data *SD_link_data_t;

typedef struct SD_link {
  SD_link_data_t sd_data; /* SD internal data */
  void *data; /* user data */

  /*char *name;*/
  /*double capacity;*/
  /*double current_bandwidth;
    double current_latency;*/
} s_SD_link_t, *SD_link_t;

/* Workstation */
typedef struct SD_workstation_data *SD_workstation_data_t;

typedef struct SD_workstation {
  SD_workstation_data_t sd_data; /* SD internal data */
  void *data; /* user data */
} s_SD_workstation_t, *SD_workstation_t;

/* Task state */
typedef enum {
  SD_SCHEDULED,
  SD_RUNNING,
  SD_DONE,
  SD_FAILED
} SD_task_state_t;

/* Task */
typedef struct SD_task {
  void *data;
  char *name;
  /*double amount;
    double remaining_amount;*/
  SD_task_state_t state;
  /* TODO: dependencies + watch */
} s_SD_task_t, *SD_task_t;

#endif
