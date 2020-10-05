/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/asserts.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <xbt/log.hpp>
#include <xbt/random.hpp>

XBT_LOG_EXTERNAL_CATEGORY(xbt);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_random, xbt, "Random");

namespace simgrid {
namespace xbt {
namespace random {

bool Random::read_state(const std::string& filename)
{
  std::ifstream file(filename);
  file >> mt19937_gen;
  file.close();
  if (file.fail())
    XBT_WARN("Could not save the RNG state to file %s.", filename.c_str());
  return not file.fail();
}

bool Random::write_state(const std::string& filename) const
{
  std::ofstream file(filename);
  file << mt19937_gen;
  file.close();
  if (file.fail())
    XBT_WARN("Could not read the RNG state from file %s.", filename.c_str());
  return not file.fail();
}

int StdRandom::uniform_int(int min, int max)
{
  std::uniform_int_distribution<> dist(min, max);
  return dist(mt19937_gen);
}

double StdRandom::uniform_real(double min, double max)
{
  std::uniform_real_distribution<> dist(min, max);
  return dist(mt19937_gen);
}

double StdRandom::exponential(double lambda)
{
  std::exponential_distribution<> dist(lambda);
  return dist(mt19937_gen);
}

double StdRandom::normal(double mean, double sd)
{
  std::normal_distribution<> dist(mean, sd);
  return dist(mt19937_gen);
}

int XbtRandom::uniform_int(int min, int max)
{
  unsigned long range  = max - min + 1;
  xbt_assert(min <= max,
             "The minimum value for the uniform integer distribution must not be greater than the maximum value");
  xbt_assert(range > 0, "Overflow in the uniform integer distribution, please use a smaller range.");
  unsigned long value;
  do {
    value = mt19937_gen();
  } while (value >= decltype(mt19937_gen)::max() - decltype(mt19937_gen)::max() % range);
  return static_cast<int>(value % range + min);
}

double XbtRandom::uniform_real(double min, double max)
{
  // This reuses Boost's uniform real distribution ideas
  constexpr unsigned long divisor = decltype(mt19937_gen)::max() - decltype(mt19937_gen)::min();
  unsigned long numerator;
  do {
    numerator = mt19937_gen() - decltype(mt19937_gen)::min();
  } while (numerator == divisor);
  return min + (max - min) * static_cast<double>(numerator) / divisor;
}

double XbtRandom::exponential(double lambda)
{
  return -1.0 / lambda * log(uniform_real(0.0, 1.0));
}

double XbtRandom::normal(double mean, double sd)
{
  double u1;
  do {
    u1 = uniform_real(0.0, 1.0);
  } while (u1 < std::numeric_limits<double>::min());
  double u2 = uniform_real(0.0, 1.0);
  double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
  return z0 * sd + mean;
}

static std::unique_ptr<Random> default_random = std::make_unique<XbtRandom>();

void set_implem_xbt()
{
  default_random = std::make_unique<XbtRandom>();
}
void set_implem_std()
{
  default_random = std::make_unique<StdRandom>();
}

void set_mersenne_seed(int seed)
{
  default_random->set_seed(seed);
}

bool read_mersenne_state(const std::string& filename)
{
  return default_random->read_state(filename);
}

bool write_mersenne_state(const std::string& filename)
{
  return default_random->write_state(filename);
}

int uniform_int(int min, int max)
{
  return default_random->uniform_int(min, max);
}

double uniform_real(double min, double max)
{
  return default_random->uniform_real(min, max);
}

double exponential(double lambda)
{
  return default_random->exponential(lambda);
}

double normal(double mean, double sd)
{
  return default_random->normal(mean, sd);
}

} // namespace random
} // namespace xbt
} // namespace simgrid
