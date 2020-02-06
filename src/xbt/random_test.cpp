/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/include/catch.hpp"
#include "xbt/log.h"
#include "xbt/random.hpp"
#include <random>
#include <cmath>

#define EpsilonApprox(a) Catch::Matchers::WithinAbs((a), 100 * std::numeric_limits<double>::epsilon())

TEST_CASE("xbt::random: Random Number Generation")
{
  SECTION("Using XBT_RNG_xbt")
  {
    simgrid::xbt::random::set_mersenne_seed(12345);
    REQUIRE_THAT(simgrid::xbt::random::exponential(25), EpsilonApprox(0.00291934352469749815));
    REQUIRE(simgrid::xbt::random::uniform_int(1, 6) == 4);
    REQUIRE_THAT(simgrid::xbt::random::uniform_real(0, 1), EpsilonApprox(0.31637556036002933979));
    REQUIRE_THAT(simgrid::xbt::random::normal(0, 2), EpsilonApprox(1.62746784853777226587));
  }

  SECTION("Using XBT_RNG_std")
  {
    std::mt19937 gen;
    gen.seed(12345);

    simgrid::xbt::random::set_implem_std();
    simgrid::xbt::random::set_mersenne_seed(12345);

    std::exponential_distribution<> distA(25);
    std::uniform_int_distribution<> distB(1, 6);
    std::uniform_real_distribution<> distC(0, 1);
    std::normal_distribution<> distD(0, 2);

    REQUIRE_THAT(simgrid::xbt::random::exponential(25), EpsilonApprox(distA(gen)));
    REQUIRE(simgrid::xbt::random::uniform_int(1, 6) == distB(gen));
    REQUIRE_THAT(simgrid::xbt::random::uniform_real(0, 1), EpsilonApprox(distC(gen)));
    REQUIRE_THAT(simgrid::xbt::random::normal(0, 2), EpsilonApprox(distD(gen)));
  }
}
