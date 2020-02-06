/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_RANDOM_HPP
#define SIMGRID_XBT_RANDOM_HPP

namespace simgrid {
namespace xbt {
namespace random {

/**
 * @brief Tells xbt/random to use the ad-hoc distribution implementation.
 */
void set_implem_xbt();

/**
 * @brief Tells xbt/random to use the standard library distribution implementation.
 */
void set_implem_std();

/**
 * @brief Sets the seed of the Mersenne-Twister RNG
 */
void set_mersenne_seed(int);

/**
 * @brief Draws an integer number uniformly in range [min, max] (min and max included)
 *
 * @param min Minimum value
 * @param max Maximum value
 */
int uniform_int(int min, int max);

/**
 * @brief Draws a real number uniformly in range [min, max) (min included, and max excluded)
 *
 * @param min Minimum value
 * @param max Maximum value
 */
double uniform_real(double min, double max);

/**
 * @brief Draws a real number according to the given exponential distribution
 *
 * @param lambda Parameter of the exponential law
 */
double exponential(double lambda);

/**
 * @brief Draws a real number according to the given normal distribution
 *
 * @param mean Mean of the normal distribution
 * @param sd Standard deviation of the normal distribution
 */
double normal(double mean, double sd);
} // namespace random
} // namespace xbt
} // namespace simgrid

#endif
