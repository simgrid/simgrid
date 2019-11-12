/* Copyright (c) 2019. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/include/catch.hpp"
#include "xbt/log.h"
#include "xbt/random.hpp"

TEST_CASE("xbt::random: Random Number Generation")
{
  SECTION("Random")
  {
    simgrid::xbt::random::set_mersenne_seed(12345);

    REQUIRE(simgrid::xbt::random::exponential(25) == 0.00291934351538427348);
    REQUIRE(simgrid::xbt::random::uniform_int(1, 6) == 4);
    REQUIRE(simgrid::xbt::random::uniform_real(0, 1) == 0.31637556043369124970);
    REQUIRE(simgrid::xbt::random::normal(0, 2) == 1.62746784745133976635);
  }
}
