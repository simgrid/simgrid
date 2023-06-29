/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/odpor_tests_private.hpp"
#include "src/mc/transition/TransitionComm.hpp"

using namespace simgrid::mc;
using namespace simgrid::mc::odpor;
using namespace simgrid::mc::udpor;

TEST_CASE("simgrid::mc::odpor::Execution: Constructing Executions")
{
  Execution execution;
  REQUIRE(execution.empty());
  REQUIRE(execution.size() == 0);
  REQUIRE_FALSE(execution.get_latest_event_handle().has_value());
}

TEST_CASE("simgrid::mc::odpor::Execution: Testing Happens-Before")
{
  SECTION("Example 1")
  {
    // We check each permutation for happens before
    // among the given actions added to the execution
    const auto a1 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a2 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    const auto a3 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);
    const auto a4 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 4);

    Execution execution;
    execution.push_transition(a1);
    execution.push_transition(a2);
    execution.push_transition(a3);
    execution.push_transition(a4);

    SECTION("Happens-before is irreflexive")
    {
      REQUIRE_FALSE(execution.happens_before(0, 0));
      REQUIRE_FALSE(execution.happens_before(1, 1));
      REQUIRE_FALSE(execution.happens_before(2, 2));
      REQUIRE_FALSE(execution.happens_before(3, 3));
    }

    SECTION("Happens-before values for each pair of events")
    {
      REQUIRE_FALSE(execution.happens_before(0, 1));
      REQUIRE_FALSE(execution.happens_before(0, 2));
      REQUIRE(execution.happens_before(0, 3));
      REQUIRE_FALSE(execution.happens_before(1, 2));
      REQUIRE_FALSE(execution.happens_before(1, 3));
      REQUIRE_FALSE(execution.happens_before(2, 3));
    }

    SECTION("Happens-before is a subset of 'occurs-before' ")
    {
      REQUIRE_FALSE(execution.happens_before(1, 0));
      REQUIRE_FALSE(execution.happens_before(2, 0));
      REQUIRE_FALSE(execution.happens_before(3, 0));
      REQUIRE_FALSE(execution.happens_before(2, 1));
      REQUIRE_FALSE(execution.happens_before(3, 1));
      REQUIRE_FALSE(execution.happens_before(3, 2));
    }
  }

  SECTION("Example 2")
  {
    const auto a1 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a2 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);
    const auto a3 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);
    const auto a4 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);

    // Notice that `a5` and `a6` are executed by the same actor; thus, although
    // the actor is executing independent actions, each still "happen-before"
    // the another

    Execution execution;
    execution.push_transition(a1);
    execution.push_transition(a2);
    execution.push_transition(a3);
    execution.push_transition(a4);

    SECTION("Happens-before is irreflexive")
    {
      REQUIRE_FALSE(execution.happens_before(0, 0));
      REQUIRE_FALSE(execution.happens_before(1, 1));
      REQUIRE_FALSE(execution.happens_before(2, 2));
      REQUIRE_FALSE(execution.happens_before(3, 3));
    }

    SECTION("Happens-before values for each pair of events")
    {
      REQUIRE_FALSE(execution.happens_before(0, 1));
      REQUIRE_FALSE(execution.happens_before(0, 2));
      REQUIRE_FALSE(execution.happens_before(0, 3));
      REQUIRE(execution.happens_before(1, 2));
      REQUIRE(execution.happens_before(1, 3));
      REQUIRE(execution.happens_before(2, 3));
    }

    SECTION("Happens-before is a subset of 'occurs-before'")
    {
      REQUIRE_FALSE(execution.happens_before(1, 0));
      REQUIRE_FALSE(execution.happens_before(2, 0));
      REQUIRE_FALSE(execution.happens_before(3, 0));
      REQUIRE_FALSE(execution.happens_before(2, 1));
      REQUIRE_FALSE(execution.happens_before(3, 1));
      REQUIRE_FALSE(execution.happens_before(3, 2));
    }
  }

  SECTION("Happens-before is transitively-closed")
  {
    SECTION("Example 1")
    {
      // Given a reversible race between events `e1` and `e3` in a simulation,
      // we assert that `e5` would be eliminated from being contained in
      // the sequence `notdep(e1, E)`
      const auto e0 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 2);
      const auto e1 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
      const auto e2 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);
      const auto e3 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
      const auto e4 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);

      Execution execution;
      execution.push_partial_execution(PartialExecution{e0, e1, e2, e3, e4});
      REQUIRE(execution.happens_before(0, 2));
      REQUIRE(execution.happens_before(2, 4));
      REQUIRE(execution.happens_before(0, 4));
    }

    SECTION("Stress testing transitivity of the happens-before relation")
    {
      // This test verifies that for each triple of events
      // in the execution, for a modestly intersting one,
      // that transitivity holds
      const auto e0  = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
      const auto e1  = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
      const auto e2  = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
      const auto e3  = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 2, -10);
      const auto e4  = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);
      const auto e5  = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 0);
      const auto e6  = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 2, -5);
      const auto e7  = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
      const auto e8  = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 0);
      const auto e9  = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 2, -10);
      const auto e10 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);

      Execution execution;
      execution.push_partial_execution(PartialExecution{e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10});

      const auto max_handle = execution.get_latest_event_handle();
      for (Execution::EventHandle e_i = 0; e_i < max_handle; ++e_i) {
        for (Execution::EventHandle e_j = 0; e_j < max_handle; ++e_j) {
          for (Execution::EventHandle e_k = 0; e_k < max_handle; ++e_k) {
            const bool e_i_before_e_j = execution.happens_before(e_i, e_j);
            const bool e_j_before_e_k = execution.happens_before(e_j, e_k);
            const bool e_i_before_e_k = execution.happens_before(e_i, e_k);
            // Logical equivalent of `e_i_before_e_j ^ e_j_before_e_k --> e_i_before_e_k`
            REQUIRE((not(e_i_before_e_j && e_j_before_e_k) || e_i_before_e_k));
          }
        }
      }
    }
  }
}

TEST_CASE("simgrid::mc::odpor::Execution: Testing Racing Events and Initials")
{
  SECTION("Example 1")
  {
    const auto a1 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2);
    const auto a3 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a4 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a5 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 2);

    Execution execution;
    execution.push_transition(a1);
    execution.push_transition(a2);
    execution.push_transition(a3);
    execution.push_transition(a4);
    execution.push_transition(a5);

    // Nothing comes before event 0
    REQUIRE(execution.get_racing_events_of(0) == std::unordered_set<Execution::EventHandle>{});

    // Events 0 and 1 are independent
    REQUIRE(execution.get_racing_events_of(1) == std::unordered_set<Execution::EventHandle>{});

    // 2 and 1 are executed by different actors and happen right after each other
    REQUIRE(execution.get_racing_events_of(2) == std::unordered_set<Execution::EventHandle>{1});

    // Although a3 and a4 are dependent, they are executed by the same actor
    REQUIRE(execution.get_racing_events_of(3) == std::unordered_set<Execution::EventHandle>{});

    // Event 4 is in a race with neither event 0 nor event 2 since those events
    // "happen-before" event 3 with which event 4 races
    //
    // Furthermore, event 1 is run by the same actor and thus also is not in a race.
    // Hence, only event 3 races with event 4
    REQUIRE(execution.get_racing_events_of(4) == std::unordered_set<Execution::EventHandle>{3});
  }

  SECTION("Example 2: Events with multiple races")
  {
    const auto a1 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2);
    const auto a3 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a4 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);

    Execution execution;
    execution.push_transition(a1);
    execution.push_transition(a2);
    execution.push_transition(a3);
    execution.push_transition(a4);

    // Nothing comes before event 0
    REQUIRE(execution.get_racing_events_of(0) == std::unordered_set<Execution::EventHandle>{});

    // Events 0 and 1 are independent
    REQUIRE(execution.get_racing_events_of(1) == std::unordered_set<Execution::EventHandle>{});

    // Event 2 is independent with event 1 and run by the same actor as event 0
    REQUIRE(execution.get_racing_events_of(2) == std::unordered_set<Execution::EventHandle>{});

    // All events are dependent with event 3, but event 0 "happens-before" event 2
    REQUIRE(execution.get_racing_events_of(3) == std::unordered_set<Execution::EventHandle>{1, 2});

    SECTION("Check initials with respect to event 1")
    {
      // First, note that v := notdep(1, execution).p == {e2}.{e3} == {e2, e3}
      // Since e2 -->_E e3, actor 3 is not an initial for E' := pre(1, execution)
      // with respect to `v`. e2, however, has nothing happening before it in dom_E(v),
      // so it is an initial of E' wrt. `v`
      const auto initial_wrt_event1 = execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{});
      REQUIRE(initial_wrt_event1 == std::unordered_set<aid_t>{1});
    }

    SECTION("Check initials with respect to event 2")
    {
      // First, note that v := notdep(1, execution).p == {}.{e3} == {e3}
      // e3 has nothing happening before it in dom_E(v), so it is an initial
      // of E' wrt. `v`
      const auto initial_wrt_event2 = execution.get_missing_source_set_actors_from(2, std::unordered_set<aid_t>{});
      REQUIRE(initial_wrt_event2 == std::unordered_set<aid_t>{3});
    }
  }

  SECTION("Example 3: Testing 'Lock' Example")
  {
    // In this example, `e0` and `e1` are lock actions that
    // are in a race. We assert that
    const auto e0 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 2);
    const auto e1 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    const auto e2 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    const auto e3 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
    const auto e4 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);

    Execution execution;
    execution.push_transition(e0);
    execution.push_transition(e1);
    execution.push_transition(e2);
    execution.push_transition(e3);
    execution.push_transition(e4);
    REQUIRE(execution.get_racing_events_of(4) == std::unordered_set<Execution::EventHandle>{0});
  }

  SECTION("Example 4: Indirect Races")
  {
    const auto e0 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto e1 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    const auto e2 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
    const auto e3 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto e4 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 6);
    const auto e5 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 2);
    const auto e6 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);
    const auto e7 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 3);
    const auto e8 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 4);
    const auto e9 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2);

    Execution execution(PartialExecution{e0, e1, e2, e3, e4, e5, e6, e7, e8, e9});

    // Nothing comes before event 0
    REQUIRE(execution.get_racing_events_of(0) == std::unordered_set<Execution::EventHandle>{});

    // Events 0 and 1 are independent
    REQUIRE(execution.get_racing_events_of(1) == std::unordered_set<Execution::EventHandle>{});

    // Events 1 and 2 are independent
    REQUIRE(execution.get_racing_events_of(2) == std::unordered_set<Execution::EventHandle>{});

    // Events 1 and 3 are independent; the rest are executed by the same actor
    REQUIRE(execution.get_racing_events_of(3) == std::unordered_set<Execution::EventHandle>{});

    // 1. Events 3 and 4 race
    // 2. Events 2 and 4 do NOT race since 2 --> 3 --> 4
    // 3. Events 1 and 4 do NOT race since 1 is independent of 4
    // 4. Events 0 and 4 do NOT race since 0 --> 2 --> 4
    REQUIRE(execution.get_racing_events_of(4) == std::unordered_set<Execution::EventHandle>{3});

    // Events 4 and 5 race; and because everyone before 4 (including 3) either
    // a) happens-before, b) races, or c) does not race with 4, 4 is the race
    REQUIRE(execution.get_racing_events_of(5) == std::unordered_set<Execution::EventHandle>{4});

    // The same logic that applied to event 5 applies to event 6
    REQUIRE(execution.get_racing_events_of(6) == std::unordered_set<Execution::EventHandle>{5});

    // The same logic applies, except that this time since events 6 and 7 are run
    // by the same actor, they don'tt actually race with one another
    REQUIRE(execution.get_racing_events_of(7) == std::unordered_set<Execution::EventHandle>{});

    // Event 8 is independent with everything
    REQUIRE(execution.get_racing_events_of(8) == std::unordered_set<Execution::EventHandle>{});

    // Event 9 is independent with events 7 and 8; event 6, however, is in race with 9.
    // The same logic above eliminates events before 6
    REQUIRE(execution.get_racing_events_of(9) == std::unordered_set<Execution::EventHandle>{6});
  }

  SECTION("Example 5: Stress testing race detection")
  {
    const auto e0  = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    const auto e1  = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
    const auto e2  = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto e3  = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 2, -10);
    const auto e4  = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);
    const auto e5  = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 0);
    const auto e6  = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 2, -5);
    const auto e7  = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 1, -5);
    const auto e8  = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 0, 4);
    const auto e9  = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 2, -10);
    const auto e10 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);

    Execution execution(PartialExecution{e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10});

    // Nothing comes before event 0
    CHECK(execution.get_racing_events_of(0) == std::unordered_set<Execution::EventHandle>{});

    // Events 0 and 1 are independent
    CHECK(execution.get_racing_events_of(1) == std::unordered_set<Execution::EventHandle>{});

    // Events 1 and 2 are executed by the same actor, even though they are dependent
    CHECK(execution.get_racing_events_of(2) == std::unordered_set<Execution::EventHandle>{});

    // Events 2 and 3 are independent while events 1 and 2 are dependent
    CHECK(execution.get_racing_events_of(3) == std::unordered_set<Execution::EventHandle>{1});

    // Event 4 is independent with everything so it can never be in a race
    CHECK(execution.get_racing_events_of(4) == std::unordered_set<Execution::EventHandle>{});

    // Event 5 is independent with event 4. Since events 2 and 3 are dependent with event 5,
    // but are independent of each other, both events are in a race with event 5
    CHECK(execution.get_racing_events_of(5) == std::unordered_set<Execution::EventHandle>{2, 3});

    // Events 5 and 6 are dependent. Everyone before 5 who's dependent with 5
    // cannot be in a race with 6; everyone before 5 who's independent with 5
    // could not race with 6
    CHECK(execution.get_racing_events_of(6) == std::unordered_set<Execution::EventHandle>{5});

    // Same goes for event 7 as for 6
    CHECK(execution.get_racing_events_of(7) == std::unordered_set<Execution::EventHandle>{6});

    // Events 5 and 8 are both run by the same actor; events in-between are independent
    CHECK(execution.get_racing_events_of(8) == std::unordered_set<Execution::EventHandle>{});

    // Event 6 blocks event 9 from racing with event 6
    CHECK(execution.get_racing_events_of(9) == std::unordered_set<Execution::EventHandle>{});

    // Event 10 is independent with everything so it can never be in a race
    CHECK(execution.get_racing_events_of(10) == std::unordered_set<Execution::EventHandle>{});
  }

  SECTION("Example with many races for one event")
  {
    const auto e0 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto e1 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2);
    const auto e2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 3);
    const auto e3 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
    const auto e4 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 5);
    const auto e5 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 6);
    const auto e6 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 7);

    Execution execution(PartialExecution{e0, e1, e2, e3, e4, e5, e6});
    REQUIRE(execution.get_racing_events_of(6) == std::unordered_set<Execution::EventHandle>{0, 1, 2, 3, 4, 5});
  }
}

TEST_CASE("simgrid::mc::odpor::Execution: Independence Tests")
{
  SECTION("Complete independence")
  {
    // Every transition is independent with every other and run by different actors. Hopefully
    // we say that the events are independent with each other...
    const auto a0 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a1 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    const auto a2 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);
    const auto a3 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 4);
    const auto a4 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 5);
    const auto a5 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 6);
    const auto a6 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 7);
    const auto a7 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 7);
    Execution execution(PartialExecution{a0, a1, a2, a3});

    REQUIRE(execution.is_independent_with_execution_of(PartialExecution{a4, a5}, a6));
    REQUIRE(execution.is_independent_with_execution_of(PartialExecution{a6, a5}, a1));
    REQUIRE(execution.is_independent_with_execution_of(PartialExecution{a6, a1}, a0));
    REQUIRE(Execution().is_independent_with_execution_of(PartialExecution{a7, a7, a1}, a3));
    REQUIRE(Execution().is_independent_with_execution_of(PartialExecution{a4, a0, a1}, a3));
    REQUIRE(Execution().is_independent_with_execution_of(PartialExecution{a0, a7, a1}, a2));

    // In this case, we notice that `a6` and `a7` are executed by the same actor.
    // Hence, a6 cannot be independent with extending the execution with a4.a5.a7.
    // Notice that we are treating *a6* as the next step of actor 7 (that is what we
    // supplied as an argument)
    REQUIRE_FALSE(execution.is_independent_with_execution_of(PartialExecution{a4, a5, a7}, a6));
  }

  SECTION("Independence is trivial with an empty extension")
  {
    REQUIRE(Execution().is_independent_with_execution_of(
        PartialExecution{}, std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1)));
  }

  SECTION("Dependencies stopping independence from being possible")
  {
    const auto a0    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a1    = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a2    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 3);
    const auto a3    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
    const auto a4    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
    const auto a5    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2);
    const auto a6    = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);
    const auto a7    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a8    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
    const auto indep = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    Execution execution(PartialExecution{a0, a1, a2, a3});

    // We see that although `a4` comes after `a1` with which it is dependent, it
    // would come before "a6" in the partial execution `w`, so it would not be independent
    REQUIRE_FALSE(execution.is_independent_with_execution_of(PartialExecution{a5, a6, a7}, a4));

    // Removing `a6` from the picture, though, gives us the independence we're looking for.
    REQUIRE(execution.is_independent_with_execution_of(PartialExecution{a5, a7}, a4));

    // BUT, we we ask about a transition which is run by the same actor, even if they would be
    // independent otherwise, we again lose independence
    REQUIRE_FALSE(execution.is_independent_with_execution_of(PartialExecution{a5, a7, a8}, a4));

    // This is a more interesting case:
    // `indep` clearly is independent with the rest of the series, even though
    // there are dependencies among the other events in the partial execution
    REQUIRE(Execution().is_independent_with_execution_of(PartialExecution{a1, a6, a7}, indep));

    // This ensures that independence is trivial with an empty partial execution
    REQUIRE(execution.is_independent_with_execution_of(PartialExecution{}, a1));
  }

  SECTION("More interesting dependencies stopping independence")
  {
    const auto e0 = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 1, 5);
    const auto e1 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
    const auto e2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
    const auto e3 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2);
    const auto e4 = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 3, 5);
    const auto e5 = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 4, 4);
    Execution execution(PartialExecution{e0, e1, e2, e3, e4, e5});

    SECTION("Action run by same actor disqualifies independence")
    {
      const auto w_1 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
      const auto w_2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
      const auto w_3 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
      const auto w_4 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 4);
      const auto w   = PartialExecution{w_1, w_2, w_3};

      const auto actor4_action  = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 4);
      const auto actor4_action2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);

      // Action `actor4_action` is independent with everything EXCEPT the last transition
      // which is executed by the same actor
      execution.is_independent_with_execution_of(w, actor4_action);

      // Action `actor4_action2` is independent with everything
      // EXCEPT the last transition which is executed by the same actor
      execution.is_independent_with_execution_of(w, actor4_action);
    }
  }
}

TEST_CASE("simgrid::mc::odpor::Execution: Initials Test")
{
  const auto a0    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
  const auto a1    = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
  const auto a2    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 3);
  const auto a3    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
  const auto a4    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
  const auto a5    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2);
  const auto a6    = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);
  const auto a7    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
  const auto a8    = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
  const auto indep = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
  Execution execution(PartialExecution{a0, a1, a2, a3});

  SECTION("Initials trivial with empty executions")
  {
    // There are no initials for an empty extension since
    // a requirement is that the actor be contained in the
    // extension itself
    REQUIRE_FALSE(execution.is_initial_after_execution_of(PartialExecution{}, 0));
    REQUIRE_FALSE(execution.is_initial_after_execution_of(PartialExecution{}, 1));
    REQUIRE_FALSE(execution.is_initial_after_execution_of(PartialExecution{}, 2));
    REQUIRE_FALSE(Execution().is_initial_after_execution_of(PartialExecution{}, 0));
    REQUIRE_FALSE(Execution().is_initial_after_execution_of(PartialExecution{}, 1));
    REQUIRE_FALSE(Execution().is_initial_after_execution_of(PartialExecution{}, 2));
  }

  SECTION("The first actor is always an initial")
  {
    // Even in the case that the action is dependent with every
    // other, if it is the first action to occur as part of the
    // extension of the execution sequence, by definition it is
    // an initial (recall that initials intuitively tell you which
    // actions can be run starting from an execution `E`; if we claim
    // to witness `E.w`, then clearly at least the first step of `w` is an initial)
    REQUIRE(execution.is_initial_after_execution_of(PartialExecution{a3}, a3->aid_));
    REQUIRE(execution.is_initial_after_execution_of(PartialExecution{a2, a3, a6}, a2->aid_));
    REQUIRE(execution.is_initial_after_execution_of(PartialExecution{a6, a1, a0}, a6->aid_));
    REQUIRE(Execution().is_initial_after_execution_of(PartialExecution{a0, a1, a2, a3}, a0->aid_));
    REQUIRE(Execution().is_initial_after_execution_of(PartialExecution{a5, a2, a8, a7, a6, a0}, a5->aid_));
    REQUIRE(Execution().is_initial_after_execution_of(PartialExecution{a8, a7, a1}, a8->aid_));
  }

  SECTION("Example: Disqualified and re-qualified after switching ordering")
  {
    // Even though actor `2` executes an independent
    // transition later on, it is blocked since its first transition
    // is dependent with actor 1's transition
    REQUIRE_FALSE(Execution().is_initial_after_execution_of(PartialExecution{a1, a5, indep}, 2));

    // However, if actor 2 executes its independent action first, it DOES become an initial
    REQUIRE(Execution().is_initial_after_execution_of(PartialExecution{a1, indep, a5}, 2));
  }

  SECTION("Example: Large partial executions")
  {
    // The record trace is `1 3 4 4 3 1 4 2`

    // Actor 1 starts the execution
    REQUIRE(Execution().is_initial_after_execution_of(PartialExecution{a1, a2, a3, a4, a6, a7, a8, indep}, 1));

    // Actor 2 all the way at the end is independent with everybody: despite
    // the tangle that comes before it, we can more it to the front with no issue
    REQUIRE(Execution().is_initial_after_execution_of(PartialExecution{a1, a2, a3, a4, a6, a7, a8, indep}, 2));

    // Actor 3 is eliminated since `a1` is dependent with `a2`
    REQUIRE_FALSE(Execution().is_initial_after_execution_of(PartialExecution{a1, a2, a3, a4, a6, a7, a8, indep}, 3));

    // Likewise with actor 4
    REQUIRE_FALSE(Execution().is_initial_after_execution_of(PartialExecution{a1, a2, a3, a4, a6, a7, a8, indep}, 4));
  }

  SECTION("Example: Stress tests for initials computation")
  {
    const auto v_1 = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 1, 3);
    const auto v_2 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
    const auto v_3 = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 2, 3);
    const auto v_4 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);
    const auto v_5 = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 3, 8);
    const auto v_6 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    const auto v_7 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);
    const auto v_8 = std::make_shared<DependentIfSameValueAction>(Transition::Type::UNKNOWN, 4, 3);
    const auto v   = PartialExecution{v_1, v_2, v_3, v_4};

    // Actor 1 being the first actor in the expansion, it is clearly an initial
    REQUIRE(Execution().is_initial_after_execution_of(v, 1));

    // Actor 2 can't be switched before actor 1 without creating an trace
    // that leads to a different state than that of `E.v := ().v := v`
    REQUIRE_FALSE(Execution().is_initial_after_execution_of(v, 2));

    // The first action of Actor 3 is independent with what comes before it, so it can
    // be moved forward. Note that this is the case even though it later executes and action
    // that's dependent with most everyone else
    REQUIRE(Execution().is_initial_after_execution_of(v, 3));

    // Actor 4 is blocked actor 3's second action `v_7`
    REQUIRE_FALSE(Execution().is_initial_after_execution_of(v, 4));
  }
}

TEST_CASE("simgrid::mc::odpor::Execution: SDPOR Backtracking Simulation")
{
  // This test case assumes that each detected race is detected to also
  // be reversible. For each reversible
  const auto e0 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
  const auto e1 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
  const auto e2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 3);
  const auto e3 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
  const auto e4 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
  const auto e5 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2);
  const auto e6 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);
  const auto e7 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);

  Execution execution;

  execution.push_transition(e0);
  REQUIRE(execution.get_racing_events_of(0) == std::unordered_set<Execution::EventHandle>{});

  execution.push_transition(e1);
  REQUIRE(execution.get_racing_events_of(1) == std::unordered_set<Execution::EventHandle>{});

  // Actor 3, since it starts the extension from event 1, clearly is an initial from there
  execution.push_transition(e2);
  REQUIRE(execution.get_racing_events_of(2) == std::unordered_set<Execution::EventHandle>{1});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{}) == std::unordered_set<aid_t>{3});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{4, 5}) ==
        std::unordered_set<aid_t>{3});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{3}) == std::unordered_set<aid_t>{});

  // e1 and e3 are in an reversible race. Actor 4 is not hindered from being moved to
  // the front since e2 is independent with e3; hence, it is an initial
  execution.push_transition(e3);
  REQUIRE(execution.get_racing_events_of(3) == std::unordered_set<Execution::EventHandle>{1});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{}) == std::unordered_set<aid_t>{4});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{3, 5}) ==
        std::unordered_set<aid_t>{4});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{4}) == std::unordered_set<aid_t>{});

  // e4 is not in a race since e3 is run by the same actor and occurs before e4
  execution.push_transition(e4);
  REQUIRE(execution.get_racing_events_of(4) == std::unordered_set<Execution::EventHandle>{});

  // e5 is independent with everything between e1 and e5, and `proc(e5) == 2`
  execution.push_transition(e5);
  REQUIRE(execution.get_racing_events_of(5) == std::unordered_set<Execution::EventHandle>{1});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{}) == std::unordered_set<aid_t>{2});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{4, 5}) ==
        std::unordered_set<aid_t>{2});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{2}) == std::unordered_set<aid_t>{});

  // Event 6 has two races: one with event 4 and one with event 5. In the latter race, actor 3 follows
  // immediately after and so is evidently a source set actor; in the former race, only actor 2 can
  // be brought to the front of the queue
  execution.push_transition(e6);
  REQUIRE(execution.get_racing_events_of(6) == std::unordered_set<Execution::EventHandle>{4, 5});
  CHECK(execution.get_missing_source_set_actors_from(4, std::unordered_set<aid_t>{}) == std::unordered_set<aid_t>{2});
  CHECK(execution.get_missing_source_set_actors_from(4, std::unordered_set<aid_t>{6, 7}) ==
        std::unordered_set<aid_t>{2});
  CHECK(execution.get_missing_source_set_actors_from(4, std::unordered_set<aid_t>{2}) == std::unordered_set<aid_t>{});
  CHECK(execution.get_missing_source_set_actors_from(5, std::unordered_set<aid_t>{}) == std::unordered_set<aid_t>{3});
  CHECK(execution.get_missing_source_set_actors_from(5, std::unordered_set<aid_t>{6, 7}) ==
        std::unordered_set<aid_t>{3});
  CHECK(execution.get_missing_source_set_actors_from(5, std::unordered_set<aid_t>{3}) == std::unordered_set<aid_t>{});

  // Finally, event e7 races with e6 and `proc(e7) = 1` is brought forward
  execution.push_transition(e7);
  REQUIRE(execution.get_racing_events_of(7) == std::unordered_set<Execution::EventHandle>{6});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{}) == std::unordered_set<aid_t>{1});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{4, 5}) ==
        std::unordered_set<aid_t>{1});
  CHECK(execution.get_missing_source_set_actors_from(1, std::unordered_set<aid_t>{1}) == std::unordered_set<aid_t>{});
}

TEST_CASE("simgrid::mc::odpor::Execution: ODPOR Smallest Sequence Tests")
{
  const auto a0 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 2);
  const auto a1 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 4);
  const auto a2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 3);
  const auto a3 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
  const auto a4 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
  const auto a5 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);

  Execution execution;
  execution.push_transition(a0);
  execution.push_transition(a1);
  execution.push_transition(a2);
  execution.push_transition(a3);
  execution.push_transition(a4);
  execution.push_transition(a5);

  SECTION("Equivalent insertions")
  {
    SECTION("Example: Eliminated through independence")
    {
      // TODO: Is this even a sensible question to ask if the two sequences
      // don't agree upon what each actor executed after `E`?
      const auto insertion =
          Execution().get_shortest_odpor_sq_subset_insertion(PartialExecution{a1, a2}, PartialExecution{a2, a5});
      REQUIRE(insertion.has_value());
      REQUIRE(insertion.value() == PartialExecution{});
    }

    SECTION("Exact match yields empty set")
    {
      const auto insertion =
          Execution().get_shortest_odpor_sq_subset_insertion(PartialExecution{a1, a2}, PartialExecution{a1, a2});
      REQUIRE(insertion.has_value());
      REQUIRE(insertion.value() == PartialExecution{});
    }
  }

  SECTION("Match against empty executions")
  {
    SECTION("Empty `v` trivially yields `w`")
    {
      auto insertion =
          Execution().get_shortest_odpor_sq_subset_insertion(PartialExecution{}, PartialExecution{a1, a5, a2});
      REQUIRE(insertion.has_value());
      REQUIRE(insertion.value() == PartialExecution{a1, a5, a2});

      insertion = execution.get_shortest_odpor_sq_subset_insertion(PartialExecution{}, PartialExecution{a1, a5, a2});
      REQUIRE(insertion.has_value());
      REQUIRE(insertion.value() == PartialExecution{a1, a5, a2});
    }

    SECTION("Empty `w` yields empty execution")
    {
      auto insertion =
          Execution().get_shortest_odpor_sq_subset_insertion(PartialExecution{a1, a4, a5}, PartialExecution{});
      REQUIRE(insertion.has_value());
      REQUIRE(insertion.value() == PartialExecution{});

      insertion = execution.get_shortest_odpor_sq_subset_insertion(PartialExecution{a1, a5, a2}, PartialExecution{});
      REQUIRE(insertion.has_value());
      REQUIRE(insertion.value() == PartialExecution{});
    }
  }
}
