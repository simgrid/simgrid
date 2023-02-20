/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"

using namespace simgrid::mc::udpor;

TEST_CASE("simgrid::mc::udpor::EventSet: Initial conditions when creating sets")
{
  SECTION("Initialization with no elements")
  {
    SECTION("Default initializer")
    {
      EventSet event_set;
      REQUIRE(event_set.size() == 0);
      REQUIRE(event_set.empty());
    }

    SECTION("Set initializer")
    {
      EventSet event_set({});
      REQUIRE(event_set.size() == 0);
      REQUIRE(event_set.empty());
    }

    SECTION("List initialization")
    {
      EventSet event_set{};
      REQUIRE(event_set.size() == 0);
      REQUIRE(event_set.empty());
    }
  }

  SECTION("Initialization with one or more elements")
  {
    UnfoldingEvent e1, e2, e3;

    SECTION("Set initializer")
    {
      EventSet event_set({&e1, &e2, &e3});
      REQUIRE(event_set.size() == 3);
      REQUIRE(event_set.contains(&e1));
      REQUIRE(event_set.contains(&e2));
      REQUIRE(event_set.contains(&e3));
      REQUIRE_FALSE(event_set.empty());
    }

    SECTION("List initialization")
    {
      UnfoldingEvent e1, e2, e3;
      EventSet event_set{&e1, &e2, &e3};
      REQUIRE(event_set.size() == 3);
      REQUIRE(event_set.contains(&e1));
      REQUIRE(event_set.contains(&e2));
      REQUIRE(event_set.contains(&e3));
      REQUIRE_FALSE(event_set.empty());
    }
  }
}

TEST_CASE("simgrid::mc::udpor::EventSet: Insertions")
{
  EventSet event_set;
  UnfoldingEvent e1, e2, e3;

  SECTION("Inserting unique elements")
  {
    event_set.insert(&e1);
    REQUIRE(event_set.size() == 1);
    REQUIRE(event_set.contains(&e1));
    REQUIRE_FALSE(event_set.empty());

    event_set.insert(&e2);
    REQUIRE(event_set.size() == 2);
    REQUIRE(event_set.contains(&e2));
    REQUIRE_FALSE(event_set.empty());

    SECTION("Check contains inserted elements")
    {
      REQUIRE(event_set.contains(&e1));
      REQUIRE(event_set.contains(&e2));
      REQUIRE_FALSE(event_set.contains(&e3));
    }
  }

  SECTION("Inserting duplicate elements")
  {
    event_set.insert(&e1);
    REQUIRE(event_set.size() == 1);
    REQUIRE(event_set.contains(&e1));
    REQUIRE_FALSE(event_set.empty());

    event_set.insert(&e1);
    REQUIRE(event_set.size() == 1);
    REQUIRE(event_set.contains(&e1));
    REQUIRE_FALSE(event_set.empty());

    SECTION("Check contains inserted elements")
    {
      REQUIRE(event_set.contains(&e1));
      REQUIRE_FALSE(event_set.contains(&e2));
      REQUIRE_FALSE(event_set.contains(&e3));
    }
  }
}

TEST_CASE("simgrid::mc::udpor::EventSet: Deletions")
{
  UnfoldingEvent e1, e2, e3, e4;
  EventSet event_set({&e1, &e2, &e3});

  SECTION("Remove an element already present")
  {
    REQUIRE(event_set.contains(&e1));

    // event_set = {e2, e3}
    event_set.remove(&e1);

    // Check that
    // 1. the size decreases by exactly 1
    // 2. the set remains unempty
    // 3. the other elements are still contained in the set
    REQUIRE(event_set.size() == 2);
    REQUIRE_FALSE(event_set.contains(&e1));
    REQUIRE(event_set.contains(&e2));
    REQUIRE(event_set.contains(&e3));
    REQUIRE_FALSE(event_set.empty());

    SECTION("Remove a single element more than once")
    {
      // event_set = {e2, e3}
      event_set.remove(&e1);
      REQUIRE(event_set.size() == 2);
      REQUIRE_FALSE(event_set.contains(&e1));
      REQUIRE(event_set.contains(&e2));
      REQUIRE(event_set.contains(&e3));
      REQUIRE_FALSE(event_set.empty());
    }

    SECTION("Remove more than one element")
    {
      // event_set = {e3}
      event_set.remove(&e2);

      REQUIRE(event_set.size() == 1);
      REQUIRE_FALSE(event_set.contains(&e1));
      REQUIRE_FALSE(event_set.contains(&e2));
      REQUIRE(event_set.contains(&e3));
      REQUIRE_FALSE(event_set.empty());

      // event_set = {}
      event_set.remove(&e3);

      REQUIRE(event_set.size() == 0);
      REQUIRE_FALSE(event_set.contains(&e1));
      REQUIRE_FALSE(event_set.contains(&e2));
      REQUIRE_FALSE(event_set.contains(&e3));
      REQUIRE(event_set.empty());
    }
  }

  SECTION("Remove an element absent from the set")
  {
    REQUIRE_FALSE(event_set.contains(&e4));

    // event_set = {e1, e2, e3}
    event_set.remove(&e4);
    REQUIRE(event_set.size() == 3);
    REQUIRE(event_set.contains(&e1));
    REQUIRE(event_set.contains(&e2));
    REQUIRE(event_set.contains(&e3));

    // Ensure e4 isn't somehow added
    REQUIRE_FALSE(event_set.contains(&e4));
    REQUIRE_FALSE(event_set.empty());
  }
}

TEST_CASE("simgrid::mc::udpor::EventSet: Set Equality") {}

TEST_CASE("simgrid::mc::udpor::EventSet: Unions and Set Difference")
{
  UnfoldingEvent e1, e2, e3, e4;

  EventSet A{&e1, &e2, &e3}, B{&e2, &e3, &e4}, C{&e2, &e3, &e4};

  SECTION("Self-union (A union A)") {}
}