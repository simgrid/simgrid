#ifndef SIMDAG_PRIVATE_H
#define SIMDAG_PRIVATE_H

#include "xbt/dict.h"
#include "simdag/simdag.h"
#include "simdag/datatypes.h"

#define CHECK_INIT_DONE() xbt_assert0(sd_global != NULL, "SD_init not called yet")

/* Global variables */

typedef struct SD_global {
  xbt_dict_t workstations; /* workstation list */
  int workstation_count; /* number of workstations */
} s_SD_global_t, *SD_global_t;

extern SD_global_t sd_global;

/* Link private data */
typedef struct SD_link_data {
  void* surf_link; /* surf object */

} s_SD_link_data_t;

/* Workstation private data */
typedef struct SD_workstation_data {
  void* surf_workstation; /* surf object */
  /* TODO: route */
} s_SD_workstation_data_t;

/* Private functions */

SD_link_t __SD_link_create(const char *name, void *surf_link, void *data);
void __SD_link_destroy(SD_link_t link);

SD_workstation_t __SD_workstation_create(void *surf_workstation, void *data);
void __SD_workstation_destroy(void *workstation);

#endif
