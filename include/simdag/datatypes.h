#ifndef SIMDAG_DATATYPES_H
#define SIMDAG_DATATYPES_H

/* Link */
typedef struct SD_link_data *SD_link_data_t;

typedef struct SD_link {
  SD_link_data_t sd_data; /* SD internal data */
  void *data; /* user data */
} s_SD_link_t, *SD_link_t;

/* Workstation */
typedef struct SD_workstation_data *SD_workstation_data_t;

typedef struct SD_workstation {
  SD_workstation_data_t sd_data; /* SD internal data */
  void *data; /* user data */
} s_SD_workstation_t, *SD_workstation_t;

/* Task state */
typedef enum {
  SD_NOT_SCHEDULED = 0, /* 0 because SD_NOT_SCHEDULED is not a valid state for SD_watch and SD_unwatch */
  SD_SCHEDULED =     0x0001,
  SD_RUNNING =       0x0002,
  SD_DONE =          0x0004,
  SD_FAILED =        0x0008 
} e_SD_task_state_t;

/* Task */
typedef struct SD_task *SD_task_t;

#endif
