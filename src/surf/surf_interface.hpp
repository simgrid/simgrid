/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_MODEL_H_
#define SURF_MODEL_H_

#include "src/simgrid/module.hpp"
#include <xbt/asserts.h>
#include <xbt/function_types.h>

#include "src/internal_config.h"
#include "src/kernel/resource/profile/Profile.hpp"

#include <cfloat>
#include <cmath>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/*********
 * Utils *
 *********/

/* user-visible parameters */
XBT_PUBLIC_DATA double sg_maxmin_precision;
XBT_PUBLIC_DATA double sg_surf_precision;
XBT_PUBLIC_DATA int sg_concurrency_limit;

extern XBT_PRIVATE std::unordered_map<std::string, simgrid::kernel::profile::Profile*> traces_set_list;

/** set of hosts for which one want to be notified if they ever restart */
inline auto& watched_hosts() // avoid static initialization order fiasco
{
  static std::set<std::string, std::less<>> value;
  return value;
}

static inline void double_update(double* variable, double value, double precision)
{
  if (false) { // debug
    fprintf(stderr, "Updating %g -= %g +- %g\n", *variable, value, precision);
    xbt_assert(value == 0.0 || value > precision);
    // Check that precision is higher than the machine-dependent size of the mantissa. If not, brutal rounding  may
    // happen, and the precision mechanism is not active...
    xbt_assert(FLT_RADIX == 2 && *variable < precision * exp2(DBL_MANT_DIG));
  }
  *variable -= value;
  if (*variable < precision)
    *variable = 0.0;
}

static inline int double_positive(double value, double precision)
{
  return (value > precision);
}

static inline int double_equals(double value1, double value2, double precision)
{
  return (fabs(value1 - value2) < precision);
}

XBT_PUBLIC void surf_vm_model_init_HL13();

#endif /* SURF_MODEL_H_ */
