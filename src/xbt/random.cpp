/* Copyright (c) 2019. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/random.hpp"
#include "xbt/asserts.h"
#include <limits>
#include <random>

namespace simgrid {
namespace xbt {
namespace random {
std::mt19937 mt19937_gen;

int uniform_int(int min, int max)
{
  unsigned long gmin   = mt19937_gen.min();
  unsigned long gmax   = mt19937_gen.max();
  unsigned long grange = gmax - gmin + 1;
  unsigned long range  = max - min + 1;
  xbt_assert(
      min < max || min == max,
      "The maximum value for the uniform integer distribution must be greater than or equal to the minimum value");
  xbt_assert(range < grange || range == grange, "The current implementation of the uniform integer distribution does "
                                                "not allow range to be higher than mt19937's range");
  unsigned long mult       = grange / range;
  unsigned long maxallowed = gmin + (mult + 1) * range - 1;
  while (true) {
    unsigned long value = mt19937_gen();
    if (value > maxallowed) {
    } else {
      return value % range + min;
    }
  }
}

double uniform_real(double min, double max)
{
  // This reuses Boost's uniform real distribution ideas
  unsigned long numerator = mt19937_gen() - mt19937_gen.min();
  unsigned long divisor   = mt19937_gen.max() - mt19937_gen.min();
  return min + (max - min) * numerator / divisor;
}

double exponential(double lambda)
{
  return -1 / lambda * log(uniform_real(0, 1));
}

double normal(double mean, double sd)
{
  double u1 = 0;
  while (u1 < std::numeric_limits<double>::min()) {
    u1 = uniform_real(0, 1);
  }
  double u2 = uniform_real(0, 1);
  double z0 = sqrt(-2.0 * log(u1)) * cos(2 * M_PI * u2);
  return z0 * sd + mean;
}

void set_mersenne_seed(int seed)
{
  mt19937_gen.seed(seed);
}

} // namespace random
} // namespace xbt
} // namespace simgrid
