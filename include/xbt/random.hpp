/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_RANDOM_HPP
#define SIMGRID_XBT_RANDOM_HPP

#include "xbt/base.h"
#include <fstream>
#include <iostream>
#include <random>
#include <string>

namespace simgrid {
namespace xbt {
namespace random {

/** A random number generator.
 *
 * It uses a std::mersenne_twister_engine (std::mt19937) and provides several distributions.
 * This interface is implemented by StdRandom and XbtRandom.
 */
class XBT_PUBLIC Random {
public:
  std::mt19937 mt19937_gen; // the random number engine

  /** @brief Build a new random number generator with default seed */
  Random() = default;
  /** @brief Build a new random number generator with given seed */
  explicit Random(int seed) : mt19937_gen(seed) {}

  virtual ~Random() = default;

  /**
   * @brief Sets the seed of the Mersenne-Twister RNG
   */
  void set_seed(int seed) { mt19937_gen.seed(seed); }

  /**
   * @brief Read the state of the Mersenne-Twister RNG from a file
   */
  bool read_state(const std::string& filename);

  /**
   * @brief Write the state of the Mersenne-Twister RNG to a file
   */
  bool write_state(const std::string& filename) const;

  /**
   * @brief Draws an integer number uniformly in range [min, max] (min and max included)
   *
   * @param min Minimum value
   * @param max Maximum value
   */
  virtual int uniform_int(int min, int max) = 0;

  /**
   * @brief Draws a real number uniformly in range [min, max) (min included, and max excluded)
   *
   * @param min Minimum value
   * @param max Maximum value
   */
  virtual double uniform_real(double min, double max) = 0;

  /**
   * @brief Draws a real number according to the given exponential distribution
   *
   * @param lambda Parameter of the exponential law
   */
  virtual double exponential(double lambda) = 0;

  /**
   * @brief Draws a real number according to the given normal distribution
   *
   * @param mean Mean of the normal distribution
   * @param sd Standard deviation of the normal distribution
   */
  virtual double normal(double mean, double sd) = 0;
};

/** A random number generator using the C++ standard library.
 *
 * Caution: reproducibility is not guaranteed across different implementations.
 */
class XBT_PUBLIC StdRandom : public Random {
public:
  using Random::Random;

  int uniform_int(int min, int max) override;
  double uniform_real(double min, double max) override;
  double exponential(double lambda) override;
  double normal(double mean, double sd) override;
};

/** A reproducible random number generator.
 *
 * Uses our own implementation of distributions to ensure reproducibility.
 */
class XBT_PUBLIC XbtRandom : public Random {
public:
  using Random::Random;

  int uniform_int(int min, int max) override;
  double uniform_real(double min, double max) override;
  double exponential(double lambda) override;
  double normal(double mean, double sd) override;
};

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
 * @brief Read the state of the Mersenne-Twister RNG from a file.
 */
bool read_mersenne_state(const std::string& filename);

/**
 * @brief Write the state of the Mersenne-Twister RNG to a file.
 */
bool write_mersenne_state(const std::string& filename);

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
