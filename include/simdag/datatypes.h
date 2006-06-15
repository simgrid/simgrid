#ifndef SIMDAG_DATATYPES_H
#define SIMDAG_DATATYPES_H

/* Link */
typedef struct SG_link_data *SG_link_data_t;

typedef struct SG_link {
  SG_link_data_t sgdata; /* SG internal data */
  void *data; /* user data */
  char *name;

  /*double capacity;*/
  /*double current_bandwidth;
    double current_latency;*/
} s_SG_link_t, *SG_link_t;

/* Workstation */
typedef struct SG_workstation_data *SG_workstation_data_t;

typedef struct SG_workstation {
  SG_workstation_data_t sgdata; /* SG internal data */
  void *data; /* user data */
  char *name;

  /*  double power;
    double available_power;*/
} s_SG_workstation_t, *SG_workstation_t;

/* Task state */
typedef enum {
  SG_SCHEDULED,
  SG_RUNNING,
  SG_DONE,
  SG_FAILED
} SG_task_state_t;

/* Task */
typedef struct SG_task {
  void *data;
  char *name;
  /*double amount;
    double remaining_amount;*/
  SG_task_state_t state;
  /* TODO: dependencies + watch */
} s_SG_task_t, *SG_task_t;

#endif
