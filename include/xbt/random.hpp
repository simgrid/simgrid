/* Copyright (c) 2019. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_RANDOM_HPP
#define SIMGRID_XBT_RANDOM_HPP

#include <random>

namespace simgrid {
namespace xbt {
namespace random {
enum xbt_random_method { XBT_RNG_xbt, XBT_RNG_std };

void use_xbt();
void use_std();
int uniform_int(int, int);
int xbt_uniform_int(int, int);
double uniform_real(double, double);
double xbt_uniform_real(double, double);
double exponential(double);
double xbt_exponential(double);
double normal(double, double);
double xbt_normal(double, double);
void set_mersenne_seed(int);
} // namespace random
} // namespace xbt
} // namespace simgrid

#endif
