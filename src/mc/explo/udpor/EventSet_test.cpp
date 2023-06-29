/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/udpor_tests_private.hpp"
#include "src/xbt/utils/iter/LazyPowerset.hpp"

using namespace simgrid::xbt;
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
    UnfoldingEvent e1;
    UnfoldingEvent e2;
    UnfoldingEvent e3;

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
  UnfoldingEvent e1;
  UnfoldingEvent e2;
  UnfoldingEvent e3;

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
  UnfoldingEvent e1;
  UnfoldingEvent e2;
  UnfoldingEvent e3;
  UnfoldingEvent e4;
  EventSet event_set({&e1, &e2, &e3});

  SECTION("Remove an element already present")
  {
    REQUIRE(event_set.contains(&e1));

    // Recall that event_set = {e2, e3}
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
      // Recall that event_set = {e2, e3}
      event_set.remove(&e1);
      REQUIRE(event_set.size() == 2);
      REQUIRE_FALSE(event_set.contains(&e1));
      REQUIRE(event_set.contains(&e2));
      REQUIRE(event_set.contains(&e3));
      REQUIRE_FALSE(event_set.empty());
    }

    SECTION("Remove more than one element")
    {
      // Recall that event_set = {e3}
      event_set.remove(&e2);

      REQUIRE(event_set.size() == 1);
      REQUIRE_FALSE(event_set.contains(&e1));
      REQUIRE_FALSE(event_set.contains(&e2));
      REQUIRE(event_set.contains(&e3));
      REQUIRE_FALSE(event_set.empty());

      // Recall that event_set = {}
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

    // Recall that event_set = {e1, e2, e3}
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
  UnfoldingEvent e1;
  UnfoldingEvent e2;
  UnfoldingEvent e3;
  UnfoldingEvent e4;
  EventSet A{&e1, &e2, &e3};
  EventSet B{&e1, &e2, &e3};
  EventSet C{&e1, &e2, &e3};

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

TEST_CASE("simgrid::mc::udpor::EventSet: Set Union Tests")
{
  UnfoldingEvent e1;
  UnfoldingEvent e2;
  UnfoldingEvent e3;
  UnfoldingEvent e4;

  // C = A + B
  EventSet A{&e1, &e2, &e3};
  EventSet B{&e2, &e3, &e4};
  EventSet C{&e1, &e2, &e3, &e4};
  EventSet D{&e1, &e3};

  SECTION("Unions with no effect")
  {
    EventSet A_copy = A;

    SECTION("Self union")
    {
      // A = A union A
      EventSet A_union = A.make_union(A);
      REQUIRE(A == A_union);

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

    SECTION("Union with a subset")
    {
      // A = A union D if D is a subset of A
      EventSet A_union = A.make_union(D);
      REQUIRE(A == A_union);

      A.form_union(D);
      REQUIRE(A == A_copy);
    }
  }

  SECTION("Unions with partial overlaps")
  {
    EventSet A_union_B = A.make_union(B);
    REQUIRE(A_union_B == C);

    A.form_union(B);
    REQUIRE(A == C);

    EventSet B_union_D = B.make_union(D);
    REQUIRE(B_union_D == C);

    B.form_union(D);
    REQUIRE(B == C);
  }

  SECTION("Set union properties")
  {
    SECTION("Union operator is symmetric")
    {
      EventSet A_union_B = A.make_union(B);
      EventSet B_union_A = B.make_union(A);
      REQUIRE(A_union_B == B_union_A);
    }

    SECTION("Union operator commutes")
    {
      // The last SECTION tested pair-wise
      // equivalence, so we only check
      // one of each pai
      EventSet AD = A.make_union(D);
      EventSet AC = A.make_union(C);
      EventSet CD = D.make_union(C);

      EventSet ADC = AD.make_union(C);
      EventSet ACD = AC.make_union(D);
      EventSet CDA = CD.make_union(A);

      REQUIRE(ADC == ACD);
      REQUIRE(ACD == CDA);

      // Test `form_union()` in the same way

      EventSet A_copy = A;
      EventSet C_copy = C;
      EventSet D_copy = D;

      A.form_union(C_copy);
      A.form_union(D_copy);

      D.form_union(A_copy);
      D.form_union(C_copy);

      C.form_union(A);
      C.form_union(D);

      REQUIRE(A == D);
      REQUIRE(C == D);
      REQUIRE(A == C);
    }
  }
}

TEST_CASE("simgrid::mc::udpor::EventSet: Set Difference Tests")
{
  UnfoldingEvent e1;
  UnfoldingEvent e2;
  UnfoldingEvent e3;
  UnfoldingEvent e4;

  // C = A + B
  // A is a subset of C
  // B is a subset of C
  // D is a subset of A and C
  // E is a subset of B and C
  // F is a subset of A, C, and D
  EventSet A{&e1, &e2, &e3};
  EventSet B{&e2, &e3, &e4};
  EventSet C{&e1, &e2, &e3, &e4};
  EventSet D{&e1, &e3};
  EventSet E{&e4};
  EventSet F{&e1};

  SECTION("Difference with no effect")
  {
    SECTION("Difference with empty set")
    {
      EventSet A_copy = A.subtracting(EventSet());
      REQUIRE(A == A_copy);

      A.subtract(EventSet());
      REQUIRE(A == A_copy);
    }

    SECTION("Difference with empty intersection")
    {
      // A intersection E = empty set
      EventSet A_copy = A.subtracting(E);
      REQUIRE(A == A_copy);

      A.subtract(E);
      REQUIRE(A == A_copy);

      EventSet D_copy = D.subtracting(E);
      REQUIRE(D == D_copy);

      D.subtract(E);
      REQUIRE(D == D_copy);
    }
  }

  SECTION("Difference with some overlap")
  {
    // A - B = {&e1} = F
    EventSet A_minus_B = A.subtracting(B);
    REQUIRE(A_minus_B == F);

    // B - D = {&e2, &e4}
    EventSet B_minus_D = B.subtracting(D);
    REQUIRE(B_minus_D == EventSet({&e2, &e4}));
  }

  SECTION("Difference with complete overlap")
  {
    SECTION("Difference with same set gives empty set")
    {
      REQUIRE(A.subtracting(A) == EventSet());
      REQUIRE(B.subtracting(B) == EventSet());
      REQUIRE(C.subtracting(C) == EventSet());
      REQUIRE(D.subtracting(D) == EventSet());
      REQUIRE(E.subtracting(E) == EventSet());
      REQUIRE(F.subtracting(F) == EventSet());
    }

    SECTION("Difference with superset gives empty set")
    {
      REQUIRE(A.subtracting(C) == EventSet());
      REQUIRE(B.subtracting(C) == EventSet());
      REQUIRE(D.subtracting(A) == EventSet());
      REQUIRE(D.subtracting(C) == EventSet());
      REQUIRE(E.subtracting(B) == EventSet());
      REQUIRE(E.subtracting(C) == EventSet());
      REQUIRE(F.subtracting(A) == EventSet());
      REQUIRE(F.subtracting(C) == EventSet());
      REQUIRE(F.subtracting(D) == EventSet());
    }
  }
}

TEST_CASE("simgrid::mc::udpor::EventSet: Subset Tests")
{
  UnfoldingEvent e1;
  UnfoldingEvent e2;
  UnfoldingEvent e3;
  UnfoldingEvent e4;

  // A is a subset of C only
  // B is a subset of C only
  // D is a subset of C and A
  // D is NOT a subset of B
  // B is NOT a subset of D
  // ...
  EventSet A{&e1, &e2, &e3};
  EventSet B{&e2, &e3, &e4};
  EventSet C{&e1, &e2, &e3, &e4};
  EventSet D{&e1, &e3};
  EventSet E{&e2, &e3};
  EventSet F{&e1, &e2, &e3};

  SECTION("Subset operator properties")
  {
    SECTION("Subset operator is not commutative")
    {
      REQUIRE(A.is_subset_of(C));
      REQUIRE_FALSE(C.is_subset_of(A));

      SECTION("Commutativity implies equality and vice versa")
      {
        REQUIRE(A.is_subset_of(F));
        REQUIRE(F.is_subset_of(A));
        REQUIRE(A == F);

        REQUIRE(C == C);
        REQUIRE(A.is_subset_of(F));
        REQUIRE(F.is_subset_of(A));
      }
    }

    SECTION("Subset operator is transitive")
    {
      REQUIRE(D.is_subset_of(A));
      REQUIRE(A.is_subset_of(C));
      REQUIRE(D.is_subset_of(C));
      REQUIRE(E.is_subset_of(B));
      REQUIRE(B.is_subset_of(C));
      REQUIRE(E.is_subset_of(C));
    }

    SECTION("Subset operator is reflexive")
    {
      REQUIRE(A.is_subset_of(A));
      REQUIRE(B.is_subset_of(B));
      REQUIRE(C.is_subset_of(C));
      REQUIRE(D.is_subset_of(D));
      REQUIRE(E.is_subset_of(E));
      REQUIRE(F.is_subset_of(F));
    }
  }
}

TEST_CASE("simgrid::mc::udpor::EventSet: Testing Configurations")
{
  // The following tests concern the given event structure:
  //                e1
  //              /    /
  //            e2      e5
  //           /  /      /
  //          e3  e4     e6
  // The tests enumerate all possible subsets of the events
  // in the structure and test whether those subsets are
  // maximal and/or valid configurations
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(0));
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(1));
  UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(2));
  UnfoldingEvent e4(EventSet({&e2}), std::make_shared<IndependentAction>(3));
  UnfoldingEvent e5(EventSet({&e1}), std::make_shared<IndependentAction>(4));
  UnfoldingEvent e6(EventSet({&e5}), std::make_shared<IndependentAction>(5));

  SECTION("Valid Configurations")
  {
    SECTION("The empty set is valid")
    {
      REQUIRE(EventSet().is_valid_configuration());
    }

    SECTION("The set with only the root event is valid")
    {
      REQUIRE(EventSet({&e1}).is_valid_configuration());
    }

    SECTION("All sets of maximal events are valid configurations")
    {
      REQUIRE(EventSet({&e1}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e3}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e4}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e5}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e5, &e6}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e5}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e5, &e6}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e3, &e4}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e3, &e5}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e4, &e5}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e4, &e5, &e6}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e3, &e4, &e5}).is_valid_configuration());
      REQUIRE(EventSet({&e1, &e2, &e3, &e4, &e5, &e6}).is_valid_configuration());
    }
  }

  SECTION("Configuration checks")
  {
    // 6 choose 0 = 1 test
    REQUIRE(EventSet().is_valid_configuration());

    // 6 choose 1 = 6 tests
    REQUIRE(EventSet({&e1}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e3}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e4}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e6}).is_valid_configuration());

    // 6 choose 2 = 15 tests
    REQUIRE(EventSet({&e1, &e2}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e3}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e4}).is_valid_configuration());
    REQUIRE(EventSet({&e1, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e3}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e4}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e3, &e4}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e3, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e3, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e4, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e4, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e5, &e6}).is_valid_configuration());

    // 6 choose 3 = 20 tests
    REQUIRE(EventSet({&e1, &e2, &e3}).is_valid_configuration());
    REQUIRE(EventSet({&e1, &e2, &e4}).is_valid_configuration());
    REQUIRE(EventSet({&e1, &e2, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e4}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e4, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e4, &e6}).is_valid_configuration());
    REQUIRE(EventSet({&e1, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e4}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e4, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e4, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e3, &e4, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e3, &e4, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e3, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e4, &e5, &e6}).is_valid_configuration());

    // 6 choose 4 = 15 tests
    REQUIRE(EventSet({&e1, &e2, &e3, &e4}).is_valid_configuration());
    REQUIRE(EventSet({&e1, &e2, &e3, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3, &e6}).is_valid_configuration());
    REQUIRE(EventSet({&e1, &e2, &e4, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e4, &e6}).is_valid_configuration());
    REQUIRE(EventSet({&e1, &e2, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e4, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e4, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e4, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e4, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e4, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e4, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e3, &e4, &e5, &e6}).is_valid_configuration());

    // 6 choose 5 = 6 tests
    REQUIRE(EventSet({&e1, &e2, &e3, &e4, &e5}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3, &e4, &e6}).is_valid_configuration());
    REQUIRE(EventSet({&e1, &e2, &e3, &e5, &e6}).is_valid_configuration());
    REQUIRE(EventSet({&e1, &e2, &e4, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e4, &e5, &e6}).is_valid_configuration());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e4, &e5, &e6}).is_valid_configuration());

    // 6 choose 6 = 1 test
    REQUIRE(EventSet({&e1, &e2, &e3, &e4, &e5, &e6}).is_valid_configuration());
  }

  SECTION("Maximal event sets")
  {
    // 6 choose 0 = 1 test
    REQUIRE(EventSet().is_maximal());

    // 6 choose 1 = 6 tests
    REQUIRE(EventSet({&e1}).is_maximal());
    REQUIRE(EventSet({&e2}).is_maximal());
    REQUIRE(EventSet({&e3}).is_maximal());
    REQUIRE(EventSet({&e4}).is_maximal());
    REQUIRE(EventSet({&e5}).is_maximal());
    REQUIRE(EventSet({&e6}).is_maximal());

    // 6 choose 2 = 15 tests
    REQUIRE_FALSE(EventSet({&e1, &e2}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e3}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e4}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e3}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e4}).is_maximal());
    REQUIRE(EventSet({&e2, &e5}).is_maximal());
    REQUIRE(EventSet({&e2, &e6}).is_maximal());
    REQUIRE(EventSet({&e3, &e4}).is_maximal());
    REQUIRE(EventSet({&e3, &e5}).is_maximal());
    REQUIRE(EventSet({&e3, &e6}).is_maximal());
    REQUIRE(EventSet({&e4, &e5}).is_maximal());
    REQUIRE(EventSet({&e4, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e5, &e6}).is_maximal());

    // 6 choose 3 = 20 tests
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e4}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e4}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e4, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e4, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e4}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e4, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e4, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e5, &e6}).is_maximal());
    REQUIRE(EventSet({&e3, &e4, &e5}).is_maximal());
    REQUIRE(EventSet({&e3, &e4, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e3, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e4, &e5, &e6}).is_maximal());

    // 6 choose 4 = 15 tests
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3, &e4}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e4, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e4, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e4, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e4, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e4, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e4, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e4, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e4, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e3, &e4, &e5, &e6}).is_maximal());

    // 6 choose 5 = 6 tests
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3, &e4, &e5}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3, &e4, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e2, &e4, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e1, &e3, &e4, &e5, &e6}).is_maximal());
    REQUIRE_FALSE(EventSet({&e2, &e3, &e4, &e5, &e6}).is_maximal());

    // 6 choose 6 = 1 test
    REQUIRE_FALSE(EventSet({&e1, &e2, &e3, &e4, &e5, &e6}).is_maximal());
  }
}

TEST_CASE("simgrid::mc::udpor::EventSet: Moving into a collection")
{
  UnfoldingEvent e1;
  UnfoldingEvent e2;
  UnfoldingEvent e3;
  UnfoldingEvent e4;

  EventSet A({&e1});
  EventSet B({&e1, &e2});
  EventSet C({&e1, &e2, &e3});
  EventSet D({&e1, &e2, &e3, &e4});

  EventSet A_copy = A;
  EventSet B_copy = B;
  EventSet C_copy = C;
  EventSet D_copy = D;

  const std::vector<const UnfoldingEvent*> actual_A = std::move(A).move_into_vector();
  const std::vector<const UnfoldingEvent*> actual_B = std::move(B).move_into_vector();
  const std::vector<const UnfoldingEvent*> actual_C = std::move(C).move_into_vector();
  const std::vector<const UnfoldingEvent*> actual_D = std::move(D).move_into_vector();

  EventSet A_copy_remade(actual_A);
  EventSet B_copy_remade(actual_B);
  EventSet C_copy_remade(actual_C);
  EventSet D_copy_remade(actual_D);

  REQUIRE(A_copy == A_copy_remade);
  REQUIRE(B_copy == B_copy_remade);
  REQUIRE(C_copy == C_copy_remade);
  REQUIRE(D_copy == D_copy_remade);
}

TEST_CASE("simgrid::mc::udpor::EventSet: Checking conflicts")
{
  // The following tests concern the given event structure:
  //                e1
  //              /    /
  //            e2      e5
  //           /  /      /
  //          e3  e4     e6
  // The tests enumerate all possible subsets of the events
  // in the structure and test whether those subsets contain
  // conflicts.
  //
  // Each test assigns different transitions to each event to
  // make the scenarios more interesting

  SECTION("No conflicts throughout the whole structure with independent actions")
  {
    UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(0));
    UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(1));
    UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(2));
    UnfoldingEvent e4(EventSet({&e2}), std::make_shared<IndependentAction>(3));
    UnfoldingEvent e5(EventSet({&e1}), std::make_shared<IndependentAction>(4));
    UnfoldingEvent e6(EventSet({&e5}), std::make_shared<IndependentAction>(5));

    // 6 choose 0 = 1 test
    CHECK(EventSet().is_conflict_free());

    // 6 choose 1 = 6 tests
    CHECK(EventSet({&e1}).is_conflict_free());
    CHECK(EventSet({&e2}).is_conflict_free());
    CHECK(EventSet({&e3}).is_conflict_free());
    CHECK(EventSet({&e4}).is_conflict_free());
    CHECK(EventSet({&e5}).is_conflict_free());
    CHECK(EventSet({&e6}).is_conflict_free());

    // 6 choose 2 = 15 tests
    CHECK(EventSet({&e1, &e2}).is_conflict_free());
    CHECK(EventSet({&e1, &e3}).is_conflict_free());
    CHECK(EventSet({&e1, &e4}).is_conflict_free());
    CHECK(EventSet({&e1, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e3}).is_conflict_free());
    CHECK(EventSet({&e2, &e4}).is_conflict_free());
    CHECK(EventSet({&e2, &e5}).is_conflict_free());
    CHECK(EventSet({&e2, &e6}).is_conflict_free());
    CHECK(EventSet({&e3, &e4}).is_conflict_free());
    CHECK(EventSet({&e3, &e5}).is_conflict_free());
    CHECK(EventSet({&e3, &e6}).is_conflict_free());
    CHECK(EventSet({&e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e5, &e6}).is_conflict_free());

    // 6 choose 3 = 20 tests
    CHECK(EventSet({&e1, &e2, &e3}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e4}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e4}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e3, &e4}).is_conflict_free());
    CHECK(EventSet({&e2, &e3, &e5}).is_conflict_free());
    CHECK(EventSet({&e2, &e3, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e2, &e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e3, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e3, &e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e3, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e4, &e5, &e6}).is_conflict_free());

    // 6 choose 4 = 15 tests
    CHECK(EventSet({&e1, &e2, &e3, &e4}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e3, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e3, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e4, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e3, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e2, &e3, &e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e3, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e4, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e3, &e4, &e5, &e6}).is_conflict_free());

    // 6 choose 5 = 6 tests
    CHECK(EventSet({&e1, &e2, &e3, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e3, &e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e3, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e4, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e4, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e3, &e4, &e5, &e6}).is_conflict_free());

    // 6 choose 6 = 1 test
    CHECK(EventSet({&e1, &e2, &e3, &e4, &e5, &e6}).is_conflict_free());
  }

  SECTION("Conflicts throughout the whole structure with dependent actions (except with DFS sets)")
  {
    // Since all actions are dependent, if a set contains a "fork" or divergent histories,
    // we expect the collections to contain conflicts
    UnfoldingEvent e1(EventSet(), std::make_shared<DependentAction>());
    UnfoldingEvent e2(EventSet({&e1}), std::make_shared<DependentAction>());
    UnfoldingEvent e3(EventSet({&e2}), std::make_shared<DependentAction>());
    UnfoldingEvent e4(EventSet({&e2}), std::make_shared<DependentAction>());
    UnfoldingEvent e5(EventSet({&e1}), std::make_shared<DependentAction>());
    UnfoldingEvent e6(EventSet({&e5}), std::make_shared<DependentAction>());

    // 6 choose 0 = 1 test
    // There are no events even to be in conflict with
    CHECK(EventSet().is_conflict_free());

    // 6 choose 1 = 6 tests
    // Sets of size 1 should have no conflicts
    CHECK(EventSet({&e1}).is_conflict_free());
    CHECK(EventSet({&e2}).is_conflict_free());
    CHECK(EventSet({&e3}).is_conflict_free());
    CHECK(EventSet({&e4}).is_conflict_free());
    CHECK(EventSet({&e5}).is_conflict_free());
    CHECK(EventSet({&e6}).is_conflict_free());

    // 6 choose 2 = 15 tests
    CHECK(EventSet({&e1, &e2}).is_conflict_free());
    CHECK(EventSet({&e1, &e3}).is_conflict_free());
    CHECK(EventSet({&e1, &e4}).is_conflict_free());
    CHECK(EventSet({&e1, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e3}).is_conflict_free());
    CHECK(EventSet({&e2, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e5, &e6}).is_conflict_free());

    // 6 choose 3 = 20 tests
    CHECK(EventSet({&e1, &e2, &e3}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e4, &e5, &e6}).is_conflict_free());

    // 6 choose 4 = 15 tests
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e4, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e4, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e4, &e5, &e6}).is_conflict_free());

    // 6 choose 5 = 6 tests
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e4, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e4, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e4, &e5, &e6}).is_conflict_free());

    // 6 choose 6 = 1 test
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e4, &e5, &e6}).is_conflict_free());
  }

  SECTION("Conditional conflicts")
  {
    UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(0));
    UnfoldingEvent e2(EventSet({&e1}), std::make_shared<ConditionallyDependentAction>(1));
    UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(2));
    UnfoldingEvent e4(EventSet({&e2}), std::make_shared<IndependentAction>(3));
    UnfoldingEvent e5(EventSet({&e1}), std::make_shared<DependentAction>(4));
    UnfoldingEvent e6(EventSet({&e5}), std::make_shared<IndependentAction>(5));

    // 6 choose 0 = 1 test
    // There are no events even to be in conflict with
    CHECK(EventSet().is_conflict_free());

    // 6 choose 1 = 6 tests
    // Sets of size 1 should still have no conflicts
    CHECK(EventSet({&e1}).is_conflict_free());
    CHECK(EventSet({&e2}).is_conflict_free());
    CHECK(EventSet({&e3}).is_conflict_free());
    CHECK(EventSet({&e4}).is_conflict_free());
    CHECK(EventSet({&e5}).is_conflict_free());
    CHECK(EventSet({&e6}).is_conflict_free());

    // 6 choose 2 = 15 tests
    CHECK(EventSet({&e1, &e2}).is_conflict_free());
    CHECK(EventSet({&e1, &e3}).is_conflict_free());
    CHECK(EventSet({&e1, &e4}).is_conflict_free());
    CHECK(EventSet({&e1, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e3}).is_conflict_free());
    CHECK(EventSet({&e2, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e5}).is_conflict_free());

    // Although e2 and e6 are not directly dependent,
    // e2 conflicts with e5 which causes e6
    CHECK_FALSE(EventSet({&e2, &e6}).is_conflict_free());
    CHECK(EventSet({&e3, &e4}).is_conflict_free());

    // Likewise, since e2 and e5 conflict and e2 causes
    // e3, so e3 and e5 conflict
    CHECK_FALSE(EventSet({&e3, &e5}).is_conflict_free());
    CHECK(EventSet({&e3, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e5, &e6}).is_conflict_free());

    // 6 choose 3 = 20 tests
    CHECK(EventSet({&e1, &e2, &e3}).is_conflict_free());
    CHECK(EventSet({&e1, &e2, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e4, &e6}).is_conflict_free());
    CHECK(EventSet({&e1, &e5, &e6}).is_conflict_free());
    CHECK(EventSet({&e2, &e3, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e3, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e4, &e5, &e6}).is_conflict_free());

    // 6 choose 4 = 15 tests
    CHECK(EventSet({&e1, &e2, &e3, &e4}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e5}).is_conflict_free());

    // e2 is dependent with e6
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e4, &e5}).is_conflict_free());
    CHECK(EventSet({&e1, &e3, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e4, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e4, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e3, &e4, &e5, &e6}).is_conflict_free());

    // 6 choose 5 = 6 tests
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e4, &e5}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e4, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e2, &e4, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e1, &e3, &e4, &e5, &e6}).is_conflict_free());
    CHECK_FALSE(EventSet({&e2, &e3, &e4, &e5, &e6}).is_conflict_free());

    // 6 choose 6 = 1 test
    CHECK_FALSE(EventSet({&e1, &e2, &e3, &e4, &e5, &e6}).is_conflict_free());
  }
}

TEST_CASE("simgrid::mc::udpor::EventSet: Topological Ordering Property Observed for Every Possible Subset")
{
  // The following tests concern the given event structure:
  //                e1
  //              /   /
  //            e2    e6
  //           /  /    /
  //          e3   /   /
  //         /  /    /
  //        e4  e5   e7
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>());
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>());
  UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>());
  UnfoldingEvent e4(EventSet({&e3}), std::make_shared<IndependentAction>());
  UnfoldingEvent e5(EventSet({&e3}), std::make_shared<IndependentAction>());
  UnfoldingEvent e6(EventSet({&e1}), std::make_shared<IndependentAction>());
  UnfoldingEvent e7(EventSet({&e2, &e6}), std::make_shared<IndependentAction>());

  const EventSet all_events{&e1, &e2, &e3, &e4, &e5, &e6, &e7};

  for (const auto& subset_of_iterators : make_powerset_iter<EventSet>(all_events)) {
    // Verify that the topological ordering property holds for every subset

    const EventSet subset = [&subset_of_iterators]() {
      EventSet subset_local;
      for (const auto& iter : subset_of_iterators) {
        subset_local.insert(*iter);
      }
      return subset_local;
    }();

    // To test this, we verify that at each point none of the events
    // that follow after any particular event `e` are contained in
    // `e`'s history
    EventSet invalid_events = subset;
    for (const auto* e : subset.get_topological_ordering()) {
      for (const auto* e_hist : History(e)) {
        if (e_hist == e)
          continue;
        REQUIRE_FALSE(invalid_events.contains(e_hist));
      }
      invalid_events.remove(e);
    }

    // To test this, we verify that at each point none of the events
    // that we've processed in the ordering are ever seen again
    // in anybody else's history
    EventSet events_seen;
    for (const auto* e : subset.get_topological_ordering_of_reverse_graph()) {
      for (const auto* e_hist : History(e)) {
        // Unlike the test above, we DO want to ensure
        // that `e` itself ALSO isn't yet seen

        // If this event has been "seen" before,
        // this implies that event `e` appears later
        // in the list than one of its ancestors
        REQUIRE_FALSE(events_seen.contains(e_hist));
      }
      events_seen.insert(e);
    }
  }
}
