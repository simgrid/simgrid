/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/udpor/udpor_tests_private.hpp"
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
    execution.push_transition(a1.get());
    execution.push_transition(a2.get());
    execution.push_transition(a3.get());
    execution.push_transition(a4.get());

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
    execution.push_transition(a1.get());
    execution.push_transition(a2.get());
    execution.push_transition(a3.get());
    execution.push_transition(a4.get());

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
    execution.push_transition(a1.get());
    execution.push_transition(a2.get());
    execution.push_transition(a3.get());
    execution.push_transition(a4.get());
    execution.push_transition(a5.get());

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
    execution.push_transition(a1.get());
    execution.push_transition(a2.get());
    execution.push_transition(a3.get());
    execution.push_transition(a4.get());

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
      const auto initial_wrt_event1 = execution.get_first_ssdpor_initial_from(1, std::unordered_set<aid_t>{});
      REQUIRE(initial_wrt_event1.has_value());
      REQUIRE(initial_wrt_event1.value() == static_cast<aid_t>(1));
    }

    SECTION("Check initials with respect to event 2")
    {
      // First, note that v := notdep(1, execution).p == {}.{e3} == {e3}
      // e3 has nothing happening before it in dom_E(v), so it is an initial
      // of E' wrt. `v`
      const auto initial_wrt_event2 = execution.get_first_ssdpor_initial_from(2, std::unordered_set<aid_t>{});
      REQUIRE(initial_wrt_event2.has_value());
      REQUIRE(initial_wrt_event2.value() == static_cast<aid_t>(3));
    }
  }

  SECTION("Example 3: Testing 'Lock' Example")
  {
    const auto a1 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 2);
    const auto a2 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    const auto a3 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
    const auto a4 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
    const auto a5 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);

    Execution execution;
    execution.push_transition(a1.get());
    execution.push_transition(a2.get());
    execution.push_transition(a3.get());
    execution.push_transition(a4.get());
    execution.push_transition(a5.get());

    REQUIRE(execution.get_racing_events_of(4) == std::unordered_set<Execution::EventHandle>{0});
  }
}