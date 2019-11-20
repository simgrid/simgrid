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
xbt_random_method current_rng = XBT_RNG_xbt;

void use_xbt()
{
  current_rng = XBT_RNG_xbt;
}
void use_std()
{
  current_rng = XBT_RNG_std;
}

int uniform_int(int min, int max)
{
  switch (current_rng) {
    case XBT_RNG_xbt:
      return xbt_uniform_int(min, max);
    case XBT_RNG_std: {
      std::uniform_int_distribution<> dist(min, max);
      return dist(mt19937_gen);
    }
    default:
      xbt_assert(false, "The uniform integer distribution is not yet supported for the current RNG.");
  }
}

int xbt_uniform_int(int min, int max)
{
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
  switch (current_rng) {
    case XBT_RNG_xbt:
      return xbt_uniform_real(min, max);
    case XBT_RNG_std: {
      std::uniform_real_distribution<> dist(min, max);
      return dist(mt19937_gen);
    }
    default:
      xbt_assert(false, "The uniform real distribution is not yet supported for the current RNG.");
  }
}

double xbt_uniform_real(double min, double max)
{
  // This reuses Boost's uniform real distribution ideas
  unsigned long numerator = mt19937_gen() - mt19937_gen.min();
  unsigned long divisor   = mt19937_gen.max() - mt19937_gen.min();
  return min + (max - min) * numerator / divisor;
}

double exponential(double lambda)
{
  switch (current_rng) {
    case XBT_RNG_xbt:
      return xbt_exponential(lambda);
    case XBT_RNG_std: {
      std::exponential_distribution<> dist(lambda);
      return dist(mt19937_gen);
    }
    default:
      xbt_assert(false, "The exponential distribution is not yet supported for the current RNG.");
  }
}

double xbt_exponential(double lambda)
{
  return -1 / lambda * log(uniform_real(0, 1));
}

double normal(double mean, double sd)
{
  switch (current_rng) {
    case XBT_RNG_xbt:
      return xbt_normal(mean, sd);
    case XBT_RNG_std: {
      std::normal_distribution<> dist(mean, sd);
      return dist(mt19937_gen);
    }
    default:
      xbt_assert(false, "The normal distribution is not yet supported for the curent RNG.");
  }
}

double xbt_normal(double mean, double sd)
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
