/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/udpor_tests_private.hpp"

using namespace simgrid::mc;
using namespace simgrid::mc::udpor;

TEST_CASE("simgrid::mc::udpor::UnfoldingEvent: Semantic Equivalence Tests")
{
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));
  UnfoldingEvent e3(EventSet({&e1}), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));
  UnfoldingEvent e4(EventSet({&e1}), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));

  UnfoldingEvent e5(EventSet({&e1, &e3, &e2}), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));
  UnfoldingEvent e6(EventSet({&e1, &e3, &e2}), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));
  UnfoldingEvent e7(EventSet({&e1, &e3, &e2}), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));

  SECTION("Equivalence is an equivalence relation")
  {
    SECTION("Equivalence is reflexive")
    {
      REQUIRE(e1 == e1);
      REQUIRE(e2 == e2);
      REQUIRE(e3 == e3);
      REQUIRE(e4 == e4);
    }

    SECTION("Equivalence is symmetric")
    {
      REQUIRE(e2 == e3);
      REQUIRE(e3 == e2);
      REQUIRE(e3 == e4);
      REQUIRE(e4 == e3);
      REQUIRE(e2 == e4);
      REQUIRE(e4 == e2);
    }

    SECTION("Equivalence is transitive")
    {
      REQUIRE(e2 == e3);
      REQUIRE(e3 == e4);
      REQUIRE(e2 == e4);
      REQUIRE(e5 == e6);
      REQUIRE(e6 == e7);
      REQUIRE(e5 == e7);
    }
  }

  SECTION("Equivalence fails with different actors")
  {
    UnfoldingEvent e1_diff_actor(EventSet(), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1, 0));
    UnfoldingEvent e2_diff_actor(EventSet({&e1}), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1, 0));
    UnfoldingEvent e5_diff_actor(EventSet({&e1, &e3, &e2}),
                                 std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1, 0));
    REQUIRE(e1 != e1_diff_actor);
    REQUIRE(e1 != e2_diff_actor);
    REQUIRE(e1 != e5_diff_actor);
  }

  SECTION("Equivalence fails with different transition types")
  {
    // NOTE: We're comparing the transition `type_` field directly. To avoid
    // modifying the `Type` enum that exists in `Transition` just for the tests,
    // we instead provide different values of `Transition::Type` to simulate
    // the different types
    UnfoldingEvent e1_diff_transition(EventSet(),
                                      std::make_shared<IndependentAction>(Transition::Type::ACTOR_JOIN, 0, 0));
    UnfoldingEvent e2_diff_transition(EventSet({&e1}),
                                      std::make_shared<IndependentAction>(Transition::Type::ACTOR_JOIN, 0, 0));
    UnfoldingEvent e5_diff_transition(EventSet({&e1, &e3, &e2}),
                                      std::make_shared<IndependentAction>(Transition::Type::ACTOR_JOIN, 0, 0));
    REQUIRE(e1 != e1_diff_transition);
    REQUIRE(e1 != e2_diff_transition);
    REQUIRE(e1 != e5_diff_transition);
  }

  SECTION("Equivalence fails with different `times_considered`")
  {
    // With a different number for `times_considered`, we know
    UnfoldingEvent e1_diff_considered(EventSet(), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 1));
    UnfoldingEvent e2_diff_considered(EventSet({&e1}),
                                      std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 1));
    UnfoldingEvent e5_diff_considered(EventSet({&e1, &e3, &e2}),
                                      std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 1));
    REQUIRE(e1 != e1_diff_considered);
    REQUIRE(e1 != e2_diff_considered);
    REQUIRE(e1 != e5_diff_considered);
  }

  SECTION("Equivalence fails with different immediate histories of events")
  {
    UnfoldingEvent e1_diff_hist(EventSet({&e2}), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));
    UnfoldingEvent e2_diff_hist(EventSet({&e3}), std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));
    UnfoldingEvent e5_diff_hist(EventSet({&e1, &e2}),
                                std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 0, 0));
    REQUIRE(e1 != e1_diff_hist);
    REQUIRE(e1 != e2_diff_hist);
    REQUIRE(e1 != e5_diff_hist);
  }
}

TEST_CASE("simgrid::mc::udpor::UnfoldingEvent: Dependency/Conflict Tests")
{
  SECTION("Properties of the relations")
  {
    // The following tests concern the given event structure:
    //                e1
    //              /   /
    //            e2    e6
    //           /  /    /
    //          e3   /   /
    //         /  /    /
    //        e4  e5   e7
    //
    // e5 and e6 are in conflict, e5 and e7 are in conflict, e2 and e6, and e2 ands e7 are in conflict
    UnfoldingEvent e1(EventSet(), std::make_shared<ConditionallyDependentAction>(0));
    UnfoldingEvent e2(EventSet({&e1}), std::make_shared<DependentAction>(0));
    UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(0));
    UnfoldingEvent e4(EventSet({&e3}), std::make_shared<ConditionallyDependentAction>(1));
    UnfoldingEvent e5(EventSet({&e3}), std::make_shared<DependentAction>(1));
    UnfoldingEvent e6(EventSet({&e1}), std::make_shared<ConditionallyDependentAction>(2));
    UnfoldingEvent e7(EventSet({&e6, &e2}), std::make_shared<ConditionallyDependentAction>(3));

    SECTION("Dependency relation properties")
    {
      SECTION("Dependency is reflexive")
      {
        REQUIRE(e2.is_dependent_with(&e2));
        REQUIRE(e5.is_dependent_with(&e5));
      }

      SECTION("Dependency is symmetric")
      {
        REQUIRE(e2.is_dependent_with(&e6));
        REQUIRE(e6.is_dependent_with(&e2));
      }

      SECTION("Dependency is NOT necessarily transitive")
      {
        REQUIRE(e1.is_dependent_with(&e5));
        REQUIRE(e5.is_dependent_with(&e7));
        REQUIRE_FALSE(e1.is_dependent_with(&e7));
      }
    }

    SECTION("Conflict relation properties")
    {
      SECTION("Conflict relation is irreflexive")
      {
        REQUIRE_FALSE(e1.conflicts_with(&e1));
        REQUIRE_FALSE(e2.conflicts_with(&e2));
        REQUIRE_FALSE(e3.conflicts_with(&e3));
        REQUIRE_FALSE(e4.conflicts_with(&e4));
        REQUIRE_FALSE(e5.conflicts_with(&e5));
        REQUIRE_FALSE(e6.conflicts_with(&e6));
        REQUIRE_FALSE(e7.conflicts_with(&e7));

        REQUIRE_FALSE(e1.immediately_conflicts_with(&e1));
        REQUIRE_FALSE(e2.immediately_conflicts_with(&e2));
        REQUIRE_FALSE(e3.immediately_conflicts_with(&e3));
        REQUIRE_FALSE(e4.immediately_conflicts_with(&e4));
        REQUIRE_FALSE(e5.immediately_conflicts_with(&e5));
        REQUIRE_FALSE(e6.immediately_conflicts_with(&e6));
        REQUIRE_FALSE(e7.immediately_conflicts_with(&e7));
      }

      SECTION("Conflict relation is symmetric")
      {
        REQUIRE(e5.conflicts_with(&e6));
        REQUIRE(e6.conflicts_with(&e5));
        REQUIRE(e5.immediately_conflicts_with(&e4));
        REQUIRE(e4.immediately_conflicts_with(&e5));
      }
    }
  }

  SECTION("Testing with no dependencies whatsoever")
  {
    // The following tests concern the given event structure:
    //                e1
    //              /   /
    //            e2    e6
    //           /  /    /
    //          e3   /   /
    //         /  /    /
    //        e4  e5   e7
    UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(0));
    UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(1));
    UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(2));
    UnfoldingEvent e4(EventSet({&e3}), std::make_shared<IndependentAction>(3));
    UnfoldingEvent e5(EventSet({&e3}), std::make_shared<IndependentAction>(4));
    UnfoldingEvent e6(EventSet({&e1}), std::make_shared<IndependentAction>(5));
    UnfoldingEvent e7(EventSet({&e6, &e2}), std::make_shared<IndependentAction>(6));

    // Since everyone's actions are independent of one another, we expect
    // that there are no conflicts between each pair of events (except with
    // the same event itself)
    SECTION("Mutual dependencies")
    {
      CHECK(e1.is_dependent_with(&e1));
      CHECK_FALSE(e1.is_dependent_with(&e2));
      CHECK_FALSE(e1.is_dependent_with(&e3));
      CHECK_FALSE(e1.is_dependent_with(&e4));
      CHECK_FALSE(e1.is_dependent_with(&e5));
      CHECK_FALSE(e1.is_dependent_with(&e6));
      CHECK_FALSE(e1.is_dependent_with(&e7));

      CHECK(e2.is_dependent_with(&e2));
      CHECK_FALSE(e2.is_dependent_with(&e3));
      CHECK_FALSE(e2.is_dependent_with(&e4));
      CHECK_FALSE(e2.is_dependent_with(&e5));
      CHECK_FALSE(e2.is_dependent_with(&e6));
      CHECK_FALSE(e2.is_dependent_with(&e7));

      CHECK(e3.is_dependent_with(&e3));
      CHECK_FALSE(e3.is_dependent_with(&e4));
      CHECK_FALSE(e3.is_dependent_with(&e5));
      CHECK_FALSE(e3.is_dependent_with(&e6));
      CHECK_FALSE(e3.is_dependent_with(&e7));

      CHECK(e4.is_dependent_with(&e4));
      CHECK_FALSE(e4.is_dependent_with(&e5));
      CHECK_FALSE(e4.is_dependent_with(&e6));
      CHECK_FALSE(e4.is_dependent_with(&e7));

      CHECK(e5.is_dependent_with(&e5));
      CHECK_FALSE(e5.is_dependent_with(&e6));
      CHECK_FALSE(e5.is_dependent_with(&e7));

      CHECK(e6.is_dependent_with(&e6));
      CHECK_FALSE(e6.is_dependent_with(&e7));

      CHECK(e7.is_dependent_with(&e7));
    }

    SECTION("Mutual conflicts")
    {
      CHECK_FALSE(e1.conflicts_with(&e1));
      CHECK_FALSE(e1.conflicts_with(&e2));
      CHECK_FALSE(e1.conflicts_with(&e3));
      CHECK_FALSE(e1.conflicts_with(&e4));
      CHECK_FALSE(e1.conflicts_with(&e5));
      CHECK_FALSE(e1.conflicts_with(&e6));
      CHECK_FALSE(e1.conflicts_with(&e7));

      CHECK_FALSE(e2.conflicts_with(&e1));
      CHECK_FALSE(e2.conflicts_with(&e2));
      CHECK_FALSE(e2.conflicts_with(&e3));
      CHECK_FALSE(e2.conflicts_with(&e4));
      CHECK_FALSE(e2.conflicts_with(&e5));
      CHECK_FALSE(e2.conflicts_with(&e6));
      CHECK_FALSE(e2.conflicts_with(&e7));

      CHECK_FALSE(e3.conflicts_with(&e1));
      CHECK_FALSE(e3.conflicts_with(&e2));
      CHECK_FALSE(e3.conflicts_with(&e3));
      CHECK_FALSE(e3.conflicts_with(&e4));
      CHECK_FALSE(e3.conflicts_with(&e5));
      CHECK_FALSE(e3.conflicts_with(&e6));
      CHECK_FALSE(e3.conflicts_with(&e7));

      CHECK_FALSE(e4.conflicts_with(&e1));
      CHECK_FALSE(e4.conflicts_with(&e2));
      CHECK_FALSE(e4.conflicts_with(&e3));
      CHECK_FALSE(e4.conflicts_with(&e4));
      CHECK_FALSE(e4.conflicts_with(&e5));
      CHECK_FALSE(e4.conflicts_with(&e6));
      CHECK_FALSE(e4.conflicts_with(&e7));

      CHECK_FALSE(e5.conflicts_with(&e1));
      CHECK_FALSE(e5.conflicts_with(&e2));
      CHECK_FALSE(e5.conflicts_with(&e3));
      CHECK_FALSE(e5.conflicts_with(&e4));
      CHECK_FALSE(e5.conflicts_with(&e5));
      CHECK_FALSE(e5.conflicts_with(&e6));
      CHECK_FALSE(e5.conflicts_with(&e7));

      CHECK_FALSE(e6.conflicts_with(&e1));
      CHECK_FALSE(e6.conflicts_with(&e2));
      CHECK_FALSE(e6.conflicts_with(&e3));
      CHECK_FALSE(e6.conflicts_with(&e4));
      CHECK_FALSE(e6.conflicts_with(&e5));
      CHECK_FALSE(e6.conflicts_with(&e6));
      CHECK_FALSE(e6.conflicts_with(&e7));

      CHECK_FALSE(e7.conflicts_with(&e1));
      CHECK_FALSE(e7.conflicts_with(&e2));
      CHECK_FALSE(e7.conflicts_with(&e3));
      CHECK_FALSE(e7.conflicts_with(&e4));
      CHECK_FALSE(e7.conflicts_with(&e5));
      CHECK_FALSE(e7.conflicts_with(&e6));
      CHECK_FALSE(e7.conflicts_with(&e7));
    }
  }

  SECTION("Testing with some conflicts")
  {
    // The following tests concern the given event structure:
    //                e1
    //              /   /
    //            e2    e6
    //           /  /    /
    //          e3   /   /
    //         /  /    /
    //        e4  e5   e7
    UnfoldingEvent e1(EventSet(), std::make_shared<DependentAction>(0));
    UnfoldingEvent e2(EventSet({&e1}), std::make_shared<DependentAction>(1));
    UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(2));
    UnfoldingEvent e4(EventSet({&e3}), std::make_shared<IndependentAction>(3));
    UnfoldingEvent e5(EventSet({&e3}), std::make_shared<IndependentAction>(4));
    UnfoldingEvent e6(EventSet({&e1}), std::make_shared<IndependentAction>(5));
    UnfoldingEvent e7(EventSet({&e6, &e2}), std::make_shared<ConditionallyDependentAction>(6));

    // Since everyone's actions are independent of one another, we expect
    // that there are no conflicts between each pair of events (except the pair
    // with the event and itself)
    SECTION("Mutual dependencies")
    {
      CHECK(e1.is_dependent_with(&e1));
      CHECK(e1.is_dependent_with(&e2));
      CHECK_FALSE(e1.is_dependent_with(&e3));
      CHECK_FALSE(e1.is_dependent_with(&e4));
      CHECK_FALSE(e1.is_dependent_with(&e5));
      CHECK_FALSE(e1.is_dependent_with(&e6));
      CHECK(e1.is_dependent_with(&e7));

      CHECK(e2.is_dependent_with(&e2));
      CHECK_FALSE(e2.is_dependent_with(&e3));
      CHECK_FALSE(e2.is_dependent_with(&e4));
      CHECK_FALSE(e2.is_dependent_with(&e5));
      CHECK_FALSE(e2.is_dependent_with(&e6));
      CHECK(e2.is_dependent_with(&e7));

      CHECK(e3.is_dependent_with(&e3));
      CHECK_FALSE(e3.is_dependent_with(&e4));
      CHECK_FALSE(e3.is_dependent_with(&e5));
      CHECK_FALSE(e3.is_dependent_with(&e6));
      CHECK_FALSE(e3.is_dependent_with(&e7));

      CHECK(e4.is_dependent_with(&e4));
      CHECK_FALSE(e4.is_dependent_with(&e5));
      CHECK_FALSE(e4.is_dependent_with(&e6));
      CHECK_FALSE(e4.is_dependent_with(&e7));

      CHECK(e5.is_dependent_with(&e5));
      CHECK_FALSE(e5.is_dependent_with(&e6));
      CHECK_FALSE(e5.is_dependent_with(&e7));

      CHECK(e6.is_dependent_with(&e6));
      CHECK_FALSE(e6.is_dependent_with(&e7));

      CHECK(e7.is_dependent_with(&e7));
    }

    SECTION("Mutual conflicts")
    {
      // Although e1 is dependent with e1, e2, and e7,
      // since they are related to one another, they are not
      // considered to be in conflict
      CHECK_FALSE(e1.conflicts_with(&e1));
      CHECK_FALSE(e1.conflicts_with(&e2));
      CHECK_FALSE(e1.conflicts_with(&e3));
      CHECK_FALSE(e1.conflicts_with(&e4));
      CHECK_FALSE(e1.conflicts_with(&e5));
      CHECK_FALSE(e1.conflicts_with(&e6));
      CHECK_FALSE(e1.conflicts_with(&e7));

      // Same goes for e2 with e2 and e7
      CHECK_FALSE(e2.conflicts_with(&e1));
      CHECK_FALSE(e2.conflicts_with(&e2));
      CHECK_FALSE(e2.conflicts_with(&e3));
      CHECK_FALSE(e2.conflicts_with(&e4));
      CHECK_FALSE(e2.conflicts_with(&e5));
      CHECK_FALSE(e2.conflicts_with(&e6));
      CHECK_FALSE(e2.conflicts_with(&e7));

      CHECK_FALSE(e3.conflicts_with(&e1));
      CHECK_FALSE(e3.conflicts_with(&e2));
      CHECK_FALSE(e3.conflicts_with(&e3));
      CHECK_FALSE(e3.conflicts_with(&e4));
      CHECK_FALSE(e3.conflicts_with(&e5));
      CHECK_FALSE(e3.conflicts_with(&e6));
      CHECK_FALSE(e3.conflicts_with(&e7));

      CHECK_FALSE(e4.conflicts_with(&e1));
      CHECK_FALSE(e4.conflicts_with(&e2));
      CHECK_FALSE(e4.conflicts_with(&e3));
      CHECK_FALSE(e4.conflicts_with(&e4));
      CHECK_FALSE(e4.conflicts_with(&e5));
      CHECK_FALSE(e4.conflicts_with(&e6));
      CHECK_FALSE(e4.conflicts_with(&e7));

      CHECK_FALSE(e5.conflicts_with(&e1));
      CHECK_FALSE(e5.conflicts_with(&e2));
      CHECK_FALSE(e5.conflicts_with(&e3));
      CHECK_FALSE(e5.conflicts_with(&e4));
      CHECK_FALSE(e5.conflicts_with(&e5));
      CHECK_FALSE(e5.conflicts_with(&e6));
      CHECK_FALSE(e5.conflicts_with(&e7));

      CHECK_FALSE(e6.conflicts_with(&e1));
      CHECK_FALSE(e6.conflicts_with(&e2));
      CHECK_FALSE(e6.conflicts_with(&e3));
      CHECK_FALSE(e6.conflicts_with(&e4));
      CHECK_FALSE(e6.conflicts_with(&e5));
      CHECK_FALSE(e6.conflicts_with(&e6));
      CHECK_FALSE(e6.conflicts_with(&e7));

      CHECK_FALSE(e7.conflicts_with(&e1));
      CHECK_FALSE(e7.conflicts_with(&e2));
      CHECK_FALSE(e7.conflicts_with(&e3));
      CHECK_FALSE(e7.conflicts_with(&e4));
      CHECK_FALSE(e7.conflicts_with(&e5));
      CHECK_FALSE(e7.conflicts_with(&e6));
      CHECK_FALSE(e7.conflicts_with(&e7));
    }
  }

  SECTION("More complicated conflicts")
  {
    // The following tests concern the given event structure:
    //                e1
    //              /   /
    //            e2    e6
    //           /      /
    //          e3      /
    //         /  /    e7
    //        e4  e5
    UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(0));
    UnfoldingEvent e2(EventSet({&e1}), std::make_shared<ConditionallyDependentAction>(1));
    UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(2));
    UnfoldingEvent e4(EventSet({&e3}), std::make_shared<IndependentAction>(3));
    UnfoldingEvent e5(EventSet({&e3}), std::make_shared<IndependentAction>(4));
    UnfoldingEvent e6(EventSet({&e1}), std::make_shared<DependentAction>(5));
    UnfoldingEvent e7(EventSet({&e6}), std::make_shared<IndependentAction>(6));

    CHECK_FALSE(e1.conflicts_with(&e1));
    CHECK_FALSE(e1.conflicts_with(&e2));
    CHECK_FALSE(e1.conflicts_with(&e3));
    CHECK_FALSE(e1.conflicts_with(&e4));
    CHECK_FALSE(e1.conflicts_with(&e5));
    CHECK_FALSE(e1.conflicts_with(&e6));
    CHECK_FALSE(e1.conflicts_with(&e7));

    // e2 conflicts with e6. Since e6 < e7,
    // e2 also conflicts with e7
    CHECK_FALSE(e2.conflicts_with(&e1));
    CHECK_FALSE(e2.conflicts_with(&e2));
    CHECK_FALSE(e2.conflicts_with(&e3));
    CHECK_FALSE(e2.conflicts_with(&e4));
    CHECK_FALSE(e2.conflicts_with(&e5));
    CHECK(e2.conflicts_with(&e6));
    REQUIRE(e2.conflicts_with(&e7));

    // e3 and e6 are dependent and unrelated, so they conflict
    CHECK_FALSE(e3.conflicts_with(&e1));
    CHECK_FALSE(e3.conflicts_with(&e2));
    CHECK_FALSE(e3.conflicts_with(&e3));
    CHECK_FALSE(e3.conflicts_with(&e4));
    CHECK_FALSE(e3.conflicts_with(&e5));
    CHECK(e3.conflicts_with(&e6));
    CHECK_FALSE(e3.conflicts_with(&e7));

    // Since e3 and e6 conflict and e3 < e4, e4 and e6 conflict
    CHECK_FALSE(e4.conflicts_with(&e1));
    CHECK_FALSE(e4.conflicts_with(&e2));
    CHECK_FALSE(e4.conflicts_with(&e3));
    CHECK_FALSE(e4.conflicts_with(&e4));
    CHECK_FALSE(e4.conflicts_with(&e5));
    CHECK(e4.conflicts_with(&e6));
    CHECK_FALSE(e4.conflicts_with(&e7));

    // Likewise for e5
    CHECK_FALSE(e5.conflicts_with(&e1));
    CHECK_FALSE(e5.conflicts_with(&e2));
    CHECK_FALSE(e5.conflicts_with(&e3));
    CHECK_FALSE(e5.conflicts_with(&e4));
    CHECK_FALSE(e5.conflicts_with(&e5));
    CHECK(e5.conflicts_with(&e6));
    CHECK_FALSE(e5.conflicts_with(&e7));

    // Conflicts are symmetric
    CHECK_FALSE(e6.conflicts_with(&e1));
    CHECK(e6.conflicts_with(&e2));
    CHECK(e6.conflicts_with(&e3));
    CHECK(e6.conflicts_with(&e4));
    CHECK(e6.conflicts_with(&e5));
    CHECK_FALSE(e6.conflicts_with(&e6));
    CHECK_FALSE(e6.conflicts_with(&e7));

    CHECK_FALSE(e7.conflicts_with(&e1));
    CHECK(e7.conflicts_with(&e2));
    CHECK_FALSE(e7.conflicts_with(&e3));
    CHECK_FALSE(e7.conflicts_with(&e4));
    CHECK_FALSE(e7.conflicts_with(&e5));
    CHECK_FALSE(e7.conflicts_with(&e6));
    CHECK_FALSE(e7.conflicts_with(&e7));
  }
}

TEST_CASE("simgrid::mc::udpor::UnfoldingEvent: Immediate Conflicts + Conflicts Stress Test")
{
  // The following tests concern the given event structure:
  //                e1
  //              /  / /
  //            e2  e3  e4
  //                 / /  /
  //                 e5   e10
  //                  /
  //                 e6
  //                 / /
  //               e7  e8
  //                    /
  //                    e9
  //
  // Immediate conflicts:
  // e2 + e3
  // e3 + e4
  // e2 + e4
  //
  // NOTE: e7 + e8 ARE NOT in immediate conflict! The reason is that
  // the set of events {e8, e6, e5, e3, e4, e1} is not a valid configuration,
  // (nor is {e7, e6, e5, e3, e4, e1})
  //
  // NOTE: e2/e3 + e10 are NOT in immediate conflict! EVEN THOUGH {e1, e4, e10}
  // is a valid configuration, {e1, e2/e3, e4} is not (since e2/e3 and e4 conflict)
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>());
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<DependentAction>());
  UnfoldingEvent e3(EventSet({&e1}), std::make_shared<DependentAction>());
  UnfoldingEvent e4(EventSet({&e1}), std::make_shared<DependentAction>());
  UnfoldingEvent e5(EventSet({&e3, &e4}), std::make_shared<IndependentAction>());
  UnfoldingEvent e6(EventSet({&e5}), std::make_shared<IndependentAction>());
  UnfoldingEvent e7(EventSet({&e6}), std::make_shared<DependentAction>());
  UnfoldingEvent e8(EventSet({&e6}), std::make_shared<DependentAction>());
  UnfoldingEvent e9(EventSet({&e8}), std::make_shared<IndependentAction>());
  UnfoldingEvent e10(EventSet({&e4}), std::make_shared<DependentAction>());

  REQUIRE(e2.immediately_conflicts_with(&e3));
  REQUIRE(e3.immediately_conflicts_with(&e4));
  REQUIRE(e2.immediately_conflicts_with(&e4));

  REQUIRE(e2.conflicts_with(&e10));
  REQUIRE(e3.conflicts_with(&e10));
  REQUIRE(e7.conflicts_with(&e8));
  REQUIRE_FALSE(e2.immediately_conflicts_with(&e10));
  REQUIRE_FALSE(e3.immediately_conflicts_with(&e10));
  REQUIRE_FALSE(e7.immediately_conflicts_with(&e8));
}