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

TEST_CASE("simgrid::mc::udpor::EventSet: Set Equality")
{
  UnfoldingEvent e1, e2, e3, e4;
  EventSet A{&e1, &e2, &e3}, B{&e1, &e2, &e3}, C{&e1, &e2, &e3};

  SECTION("Equality implies containment")
  {
    REQUIRE(A == B);

    for (const auto& e : A) {
      REQUIRE(B.contains(e));
    }

    for (const auto& e : B) {
      REQUIRE(A.contains(e));
    }
  }

  SECTION("Containment implies equality")
  {
    for (const auto& e : A) {
      REQUIRE(B.contains(e));
    }

    for (const auto& e : C) {
      REQUIRE(C.contains(e));
    }

    REQUIRE(A == C);
  }

  SECTION("Equality is an equivalence relation")
  {
    // Reflexive
    REQUIRE(A == A);
    REQUIRE(B == B);
    REQUIRE(C == C);

    // Symmetric
    REQUIRE(A == B);
    REQUIRE(B == A);
    REQUIRE(A == C);
    REQUIRE(C == A);
    REQUIRE(B == C);
    REQUIRE(C == B);

    // Transitive
    REQUIRE(A == B);
    REQUIRE(B == C);
    REQUIRE(A == C);
  }

  SECTION("Equality after copy (assignment + constructor)")
  {
    EventSet A_copy = A;
    EventSet A_copy2(A);

    REQUIRE(A == A_copy);
    REQUIRE(A == A_copy2);
  }

  SECTION("Equality after move constructor")
  {
    EventSet A_copy = A;
    EventSet A_move(std::move(A));
    REQUIRE(A_move == A_copy);
  }

  SECTION("Equality after move-assignment")
  {
    EventSet A_copy = A;
    EventSet A_move = std::move(A);
    REQUIRE(A_move == A_copy);
  }
}

TEST_CASE("simgrid::mc::udpor::EventSet: Unions and Set Difference")
{
  UnfoldingEvent e1, e2, e3, e4;

  // C = A + B
  EventSet A{&e1, &e2, &e3}, B{&e2, &e3, &e4}, C{&e1, &e2, &e3, &e4}, D{&e1, &e3};

  SECTION("Unions with no effect")
  {
    EventSet A_copy = A;

    SECTION("Self union")
    {
      // A = A union A
      EventSet A_union = A.make_union(A);
      REQUIRE(A == A_copy);

      A.form_union(A);
      REQUIRE(A == A_copy);
    }

    SECTION("Union with empty set")
    {
      // A = A union empty set
      EventSet A_union = A.make_union(EventSet());
      REQUIRE(A == A_union);

      A.form_union(EventSet());
      REQUIRE(A == A_copy);
    }

    SECTION("Union with an equivalent set")
    {
      // A = A union B if B == A
      EventSet A_equiv{&e1, &e2, &e3};
      REQUIRE(A == A_equiv);

      EventSet A_union = A.make_union(A_equiv);
      REQUIRE(A_union == A_copy);

      A.form_union(A_equiv);
      REQUIRE(A == A_copy);
    }
  }

  SECTION("Unions with overlaps")
  {
    EventSet A_union_B = A.make_union(B);
    REQUIRE(A_union_B == C);

    A.form_union(B);
    REQUIRE(A == C);
  }
}