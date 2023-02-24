/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"

using namespace simgrid::mc::udpor;

TEST_CASE("simgrid::mc::udpor::Configuration: Constructing Configurations")
{
  // The following tests concern the given event structure:
  //                e1
  //              /
  //            e2
  //           /
  //          e3
  //         /  /
  //        e4   e5
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e3{&e2};
  UnfoldingEvent e4{&e3}, e5{&e3};

  SECTION("Creating a configuration without events")
  {
    Configuration C;
    REQUIRE(C.get_events().empty());
    REQUIRE(C.get_latest_event() == nullptr);
  }

  SECTION("Creating a configuration with events")
  {
    // 5 choose 0 = 1 test
    REQUIRE_NOTHROW(Configuration({&e1}));

    // 5 choose 1 = 5 tests
    REQUIRE_THROWS_AS(Configuration({&e2}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e3}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e4}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e5}), std::invalid_argument);

    // 5 choose 2 = 10 tests
    REQUIRE_NOTHROW(Configuration({&e1, &e2}));
    REQUIRE_THROWS_AS(Configuration({&e1, &e3}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e1, &e4}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e1, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e2, &e3}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e2, &e4}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e2, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e3, &e4}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e3, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e4, &e5}), std::invalid_argument);

    // 5 choose 3 = 10 tests
    REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3}));
    REQUIRE_THROWS_AS(Configuration({&e1, &e2, &e4}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e1, &e2, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e1, &e3, &e4}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e1, &e3, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e1, &e4, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e2, &e3, &e4}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e2, &e3, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e2, &e4, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e3, &e4, &e5}), std::invalid_argument);

    // 5 choose 4 = 5 tests
    REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3, &e4}));
    REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3, &e5}));
    REQUIRE_THROWS_AS(Configuration({&e1, &e2, &e4, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e1, &e3, &e4, &e5}), std::invalid_argument);
    REQUIRE_THROWS_AS(Configuration({&e2, &e3, &e4, &e5}), std::invalid_argument);

    // 5 choose 5 = 1 test
    REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3, &e4, &e5}));
  }
}

TEST_CASE("simgrid::mc::udpor::Configuration: Adding Events")
{
  // The following tests concern the given event structure:
  //                e1
  //              /
  //            e2
  //           /
  //         /  /
  //        e3   e4
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e3{&e2};
  UnfoldingEvent e4{&e2};

  REQUIRE_THROWS_AS(Configuration().add_event(nullptr), std::invalid_argument);
  REQUIRE_THROWS_AS(Configuration().add_event(&e2), std::invalid_argument);
  REQUIRE_THROWS_AS(Configuration().add_event(&e3), std::invalid_argument);
  REQUIRE_THROWS_AS(Configuration().add_event(&e4), std::invalid_argument);
  REQUIRE_THROWS_AS(Configuration({&e1}).add_event(&e3), std::invalid_argument);
  REQUIRE_THROWS_AS(Configuration({&e1}).add_event(&e4), std::invalid_argument);

  REQUIRE_NOTHROW(Configuration().add_event(&e1));
  REQUIRE_NOTHROW(Configuration({&e1}).add_event(&e1));
  REQUIRE_NOTHROW(Configuration({&e1}).add_event(&e2));
  REQUIRE_NOTHROW(Configuration({&e1, &e2}).add_event(&e1));
  REQUIRE_NOTHROW(Configuration({&e1, &e2}).add_event(&e2));
  REQUIRE_NOTHROW(Configuration({&e1, &e2}).add_event(&e3));
  REQUIRE_NOTHROW(Configuration({&e1, &e2}).add_event(&e4));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3}).add_event(&e1));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3}).add_event(&e2));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3}).add_event(&e3));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3}).add_event(&e4));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e4}).add_event(&e1));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e4}).add_event(&e2));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e4}).add_event(&e3));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e4}).add_event(&e4));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3, &e4}).add_event(&e1));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3, &e4}).add_event(&e2));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3, &e4}).add_event(&e3));
  REQUIRE_NOTHROW(Configuration({&e1, &e2, &e3, &e4}).add_event(&e4));
}
