/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/api/ClockVector.hpp"
#include "src/mc/api/Epoch.hpp"

using namespace simgrid::mc;

TEST_CASE("simgrid::mc::Aid: values out of range are rejected", "[mc][basic_types]")
{
  constexpr unsigned last = static_config::max_threads - 2; // max_threads - 1 is the INVALID value
  REQUIRE(Aid(last).value() == last);
  REQUIRE_THROWS_AS(Aid(last + 1), AidCannotBeAboveMaxThreads);
  REQUIRE_THROWS_AS(Aid(10 * last), AidCannotBeAboveMaxThreads);
  REQUIRE_THROWS_AS(Aid(-1), AidCannotBeNegative);

  Aid aid(last - 1);
  REQUIRE(aid++ == Aid(last - 1));
  REQUIRE(aid == Aid(last));
  REQUIRE_THROWS_AS(aid++, AidCannotBeAboveMaxThreads);
  REQUIRE(aid == Aid(last));

  Aid invalid;
  REQUIRE_THROWS_AS(invalid++, InvalidAid);
}

TEST_CASE("simgrid::mc::Clock: values out of range are rejected", "[mc][basic_types]")
{
  REQUIRE_THROWS_AS(Clock(~0u), InvalidClock);
  REQUIRE_THROWS_AS(Clock(-1), InvalidClock);

  ClockVector cv;
  REQUIRE_THROWS_AS(cv[Aid::INVALID] = Clock(5), InvalidAid);
  REQUIRE(not Clock::INVALID.has_value());
}

TEST_CASE("simgrid::mc::Epoch: clocks that do not fit are rejected", "[mc][basic_types]")
{
  constexpr unsigned clock_limit = 1u << (31 - __builtin_ctz(static_config::max_threads));
  REQUIRE(Epoch(Aid(3), Clock(clock_limit - 1)).get_clock() == Clock(clock_limit - 1));
  REQUIRE(Epoch(Aid(3), Clock(clock_limit - 1)).get_aid() == Aid(3));
  REQUIRE_THROWS_AS(Epoch(Aid(3), Clock(clock_limit)), InvalidClock);
  REQUIRE_THROWS_AS(Epoch(Aid::INVALID, Clock(1)), InvalidAid);
}
