/* Copyright (c) 2019. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/include/catch.hpp"
#include "xbt/log.h"
#include "xbt/random.hpp"
#include <random>
#include <cmath>

TEST_CASE("xbt::random: Random Number Generation")
{
  SECTION("Using XBT_RNG_xbt")
  {
    simgrid::xbt::random::set_mersenne_seed(12345);

    REQUIRE(fabs(simgrid::xbt::random::exponential(25) - 0.00291934351538427348) <= 1e-14);
    REQUIRE(simgrid::xbt::random::uniform_int(1, 6) == 4);
    REQUIRE(fabs(simgrid::xbt::random::uniform_real(0, 1) - 0.31637556043369124970) <= 1e-14);
    REQUIRE(fabs(simgrid::xbt::random::normal(0, 2) - 1.62746784745133976635) <= 1e-14);
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

    REQUIRE(fabs(simgrid::xbt::random::exponential(25) - distA(gen)) <= 1e-14);
    REQUIRE(simgrid::xbt::random::uniform_int(1, 6) == distB(gen));
    REQUIRE(fabs(simgrid::xbt::random::uniform_real(0, 1) - distC(gen)) <= 1e-14);
    REQUIRE(fabs(simgrid::xbt::random::normal(0, 2) - distD(gen)) <= 1e-14);
  }
}
