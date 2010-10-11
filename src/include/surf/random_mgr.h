/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_RMGR_H
#define _SURF_RMGR_H

#include "xbt/heap.h"
#include "xbt/dict.h"
#include <stdlib.h>
#include <math.h>

SG_BEGIN_DECL()

typedef enum { NONE, DRAND48, RAND } Generator;

typedef struct random_data_desc {
  long int seed;
  double max, min;
  double mean, std;             /* note: mean and standard deviation are normalized */
  Generator generator;
} s_random_data_t, *random_data_t;

XBT_PUBLIC_DATA(xbt_dict_t) random_data_list;

XBT_PUBLIC(double) random_generate(random_data_t random);
XBT_PUBLIC(random_data_t) random_new(Generator generator, long int seed,
                                     double min, double max, double mean,
                                     double stdDeviation);

SG_END_DECL()
#endif                          /* _SURF_RMGR_H */
