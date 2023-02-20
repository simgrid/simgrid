/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MATH_UTILS_H_
#define SIMGRID_MATH_UTILS_H_

#include <xbt/asserts.h>
#include <xbt/function_types.h>

#include <cfloat>
#include <math.h>

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

#endif /* SIMGRID_MATH_UTILS_H_ */
