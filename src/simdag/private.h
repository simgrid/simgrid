#ifndef SIMDAG_PRIVATE_H
#define SIMDAG_PRIVATE_H

#include "xbt/dict.h"
#include "simdag/simdag.h"
#include "simdag/datatypes.h"

/* Global variables */

typedef struct SG_global {
  xbt_dict_t workstations; /* workstation list */
  int workstation_count; /* number of workstations */
} s_SG_global_t, *SG_global_t;

extern SG_global_t sg_global;

/* Link private data */
typedef struct SG_link_data {
  void* surf_link; /* surf object */

} s_SG_link_data_t;

/* Workstation private data */
typedef struct SG_workstation_data {
  void* surf_workstation; /* surf object */
  /* TODO: route */
} s_SG_workstation_data_t;

/* Private functions */

SG_link_t __SG_link_create(const char *name, void *surf_link, void *data);
void __SG_link_destroy(SG_link_t link);

SG_workstation_t __SG_workstation_create(const char *name, void *surf_workstation, void *data);
void __SG_workstation_destroy(SG_workstation_t workstation);

#endif
