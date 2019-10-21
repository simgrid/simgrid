/* Copyright (c) 2019. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_RANDOM_HPP
#define SIMGRID_XBT_RANDOM_HPP

#include <random>

namespace simgrid {
namespace xbt {
namespace random {
int uniform_int(int, int);
double uniform_real(double, double);
double exponential(double);
} // namespace random
} // namespace xbt
} // namespace simgrid

#endif
