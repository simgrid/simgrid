/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_DATATYPES_H_
#define SIMGRID_DATATYPES_H_

#include "simgrid/forward.h"

struct vm_params {
  double max_downtime;
  double dp_intensity; // Percentage of pages that get dirty compared to netspeed [0;1] bytes per 1 flop execution
  double dp_cap;
  double mig_speed; // Set migration speed
};

typedef struct vm_params  s_vm_params_t;
typedef struct vm_params* vm_params_t;

#endif /* SIMGRID_DATATYPES_H_ */
