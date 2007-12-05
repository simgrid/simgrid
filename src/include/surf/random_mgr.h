#ifndef _SURF_RMGR_H
#define _SURF_RMGR_H

#include "xbt/heap.h"
#include "xbt/dict.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

SG_BEGIN_DECL()

typedef enum {NONE, DRAND48, RAND} Generator;

typedef struct random_data_desc {
  int max, min, mean, stdDeviation;
  Generator generator;
} s_random_data_t, *random_data_t;

XBT_PUBLIC_DATA(xbt_dict_t) random_data_list;

XBT_PUBLIC(float) random_generate(random_data_t random);
XBT_PUBLIC(random_data_t) random_new(int generator, int min, int max, int mean, int stdDeviation);

SG_END_DECL()

#endif				/* _SURF_RMGR_H */
