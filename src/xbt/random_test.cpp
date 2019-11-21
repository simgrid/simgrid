/* Copyright (c) 2019. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/include/catch.hpp"
#include "xbt/log.h"
#include "xbt/random.hpp"
#include <random>
#include <cmath>

#define EPSILON (100*std::numeric_limits<double>::epsilon())

TEST_CASE("xbt::random: Random Number Generation")
{
  SECTION("Using XBT_RNG_xbt")
  {
    simgrid::xbt::random::set_mersenne_seed(12345);
    REQUIRE(simgrid::xbt::random::exponential(25) == Approx(0.00291934351538427348).epsilon(EPSILON));
    REQUIRE(simgrid::xbt::random::uniform_int(1, 6) == 4);
    REQUIRE(simgrid::xbt::random::uniform_real(0, 1) == Approx(0.31637556043369124970).epsilon(EPSILON));
    REQUIRE(simgrid::xbt::random::normal(0, 2) == Approx(1.62746784745133976635).epsilon(EPSILON));
  }

  SECTION("Using XBT_RNG_std")
  {
    std::mt19937 gen;
    gen.seed(12345);

    simgrid::xbt::random::set_mersenne_seed(12345);
    simgrid::xbt::random::set_implem_std();

    std::exponential_distribution<> distA(25);
    std::uniform_int_distribution<> distB(1, 6);
    std::uniform_real_distribution<> distC(0, 1);
    std::normal_distribution<> distD(0, 2);

    REQUIRE(simgrid::xbt::random::exponential(25) == Approx(distA(gen)).epsilon(EPSILON));
    REQUIRE(simgrid::xbt::random::uniform_int(1, 6) == distB(gen));
    REQUIRE(simgrid::xbt::random::uniform_real(0, 1) == Approx(distC(gen)).epsilon(EPSILON));
    REQUIRE(simgrid::xbt::random::normal(0, 2) == Approx(distD(gen)).epsilon(EPSILON));
  }
}
