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
enum xbt_random_implem { XBT_RNG_xbt, XBT_RNG_std };
static xbt_random_implem rng_implem = XBT_RNG_xbt;

static std::mt19937 mt19937_gen;

void set_implem_xbt()
{
  rng_implem = XBT_RNG_xbt;
}
void set_implem_std()
{
  rng_implem = XBT_RNG_std;
}
void set_mersenne_seed(int seed)
{
  mt19937_gen.seed(seed);
}

int uniform_int(int min, int max)
{
  if (rng_implem == XBT_RNG_std) {
    std::uniform_int_distribution<> dist(min, max);
    return dist(mt19937_gen);
    }

  unsigned long range  = max - min + 1;
  unsigned long value  = mt19937_gen();
  xbt_assert(range > 0, "Overflow in the uniform integer distribution, please use a smaller range.");
  xbt_assert(
      min <= max,
      "The maximum value for the uniform integer distribution must be greater than or equal to the minimum value");
  return value % range + min;
}

double uniform_real(double min, double max)
{
  if (rng_implem == XBT_RNG_std) {
    std::uniform_real_distribution<> dist(min, max);
    return dist(mt19937_gen);
    }

  // This reuses Boost's uniform real distribution ideas
  unsigned long numerator = mt19937_gen() - mt19937_gen.min();
  unsigned long divisor   = mt19937_gen.max() - mt19937_gen.min();
  return min + (max - min) * numerator / divisor;
}

double exponential(double lambda)
{
  if (rng_implem == XBT_RNG_std) {
    std::exponential_distribution<> dist(lambda);
    return dist(mt19937_gen);
    }

  return -1 / lambda * log(uniform_real(0, 1));
}

double normal(double mean, double sd)
{
  if (rng_implem == XBT_RNG_std) {
    std::normal_distribution<> dist(mean, sd);
    return dist(mt19937_gen);
  }

  double u1 = 0;
  while (u1 < std::numeric_limits<double>::min()) {
    u1 = uniform_real(0, 1);
  }
  double u2 = uniform_real(0, 1);
  double z0 = sqrt(-2.0 * log(u1)) * cos(2 * M_PI * u2);
  return z0 * sd + mean;
}

} // namespace random
} // namespace xbt
} // namespace simgrid
