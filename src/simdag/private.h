#ifndef SIMDAG_PRIVATE_H
#define SIMDAG_PRIVATE_H

#include "xbt/dict.h"

typedef struct SG_global {
  xbt_dict_t workstations; /* workstation list */
  int workstation_count; /* number of workstations */
} s_SG_global_t, *SG_global_t;

extern SG_global_t sg_global;

#endif
