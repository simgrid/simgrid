/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Unit tests of RaceDetector, which applies the memory accesses carried by the events of an execution to SlabTrack.
 *
 * The executions are built by hand with the test transitions of odpor_tests_private.hpp: a DependentAction depends on
 * every other transition (it acts as a synchronization), while an IndependentAction depends on nothing but the
 * transitions of its own actor. The accesses carried by an event happened before it, after the previous event of the
 * same actor. The tests check the verdicts (races found through the happens-before relation, read-then-write races,
 * no confusion between pages) and the rebuild of the shadow memory after a backtrack. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/odpor/RaceDetector.hpp"
#include "src/mc/explo/odpor/odpor_tests_private.hpp"
#include "src/mc/mc_exit.hpp"

using namespace simgrid::mc;
using namespace simgrid::mc::odpor;
using namespace simgrid::mc::udpor;
using simgrid::mc::smemory::MemOpType;

alignas(4096) static char memory_pages[2][4096];
static const auto page0 = reinterpret_cast<uintptr_t>(memory_pages[0]);
static const auto page1 = reinterpret_cast<uintptr_t>(memory_pages[1]);

// Builds a transition of the given kind, carrying one memory access (unless size is 0)
template <typename Action>
static TransitionPtr action(unsigned actor, MemOpType type = MemOpType::Read, uintptr_t where = 0,
                            unsigned short size = 0)
{
  TransitionPtr t = new Action(Transition::Type::UNKNOWN, actor);
  if (size > 0)
    t->get_memory_trace().create_memory_access(type, where, size);
  return t;
}

TEST_CASE("simgrid::mc::odpor::RaceDetector: verdicts", "[mc][RaceDetector]")
{
  Execution exec;
  RaceDetector detector;

  SECTION("Unsynchronized writes race")
  {
    exec.push_transition(action<IndependentAction>(1, MemOpType::Write, page0 + 8, 4));
    detector.check(exec);
    exec.push_transition(action<IndependentAction>(2, MemOpType::Write, page0 + 8, 4));
    try {
      detector.check(exec);
      FAIL("The race was not detected");
    } catch (const McDataRace& race) {
      REQUIRE(race.first_mem_op_ == Epoch(Aid(1u), Clock(1u))); // Carried by the event of index 0
      REQUIRE(race.second_mem_op_ == Epoch(Aid(2u), Clock(2u)));
      REQUIRE(race.location_ == page0 + 8);
    }
  }

  SECTION("Read then write race")
  {
    exec.push_transition(action<IndependentAction>(1, MemOpType::Read, page0 + 8, 4));
    detector.check(exec);
    exec.push_transition(action<IndependentAction>(2, MemOpType::Write, page0 + 8, 4));
    try {
      detector.check(exec);
      FAIL("The race was not detected");
    } catch (const McDataRace& race) {
      REQUIRE(race.first_mem_type_ == MemOpType::Read);
      REQUIRE(race.second_mem_type_ == MemOpType::Write);
    }
  }

  SECTION("Same offset on two pages does not race")
  {
    exec.push_transition(action<IndependentAction>(1, MemOpType::Write, page0 + 8, 4));
    exec.push_transition(action<IndependentAction>(2, MemOpType::Write, page1 + 8, 4));
    REQUIRE_NOTHROW(detector.check(exec));
  }

  SECTION("A synchronization orders the accesses that come after it")
  {
    // Actor 1 writes then synchronizes. Actor 2 synchronizes, then writes: its write is carried by its next event.
    exec.push_transition(action<DependentAction>(1, MemOpType::Write, page0 + 8, 4));
    exec.push_transition(action<DependentAction>(2));
    exec.push_transition(action<IndependentAction>(2, MemOpType::Write, page0 + 8, 4));
    REQUIRE_NOTHROW(detector.check(exec));
  }

  SECTION("The accesses before a synchronization are not ordered by it")
  {
    // Actor 2 writes before synchronizing: its write is carried by the synchronization event itself
    exec.push_transition(action<DependentAction>(1, MemOpType::Write, page0 + 8, 4));
    exec.push_transition(action<DependentAction>(2, MemOpType::Write, page0 + 8, 4));
    REQUIRE_THROWS_AS(detector.check(exec), McDataRace);
  }
}

TEST_CASE("simgrid::mc::odpor::RaceDetector: backtracking", "[mc][RaceDetector]")
{
  Execution exec;
  RaceDetector detector;

  SECTION("The accesses of a removed event are forgotten")
  {
    exec.push_transition(action<IndependentAction>(1, MemOpType::Write, page0 + 8, 4));
    exec.push_transition(action<IndependentAction>(1, MemOpType::Write, page0 + 64, 4));
    detector.check(exec);

    // Backtrack: actor 2 writes where actor 1 wrote in the removed event, which does not race anymore
    exec.remove_last_event();
    exec.push_transition(action<IndependentAction>(2, MemOpType::Write, page0 + 64, 4));
    REQUIRE_NOTHROW(detector.check(exec));
    REQUIRE(detector.get_rebuild_count() == 1);

    // The accesses of the event that was kept are still known
    exec.push_transition(action<IndependentAction>(3, MemOpType::Write, page0 + 8, 4));
    REQUIRE_THROWS_AS(detector.check(exec), McDataRace);
  }

  SECTION("Removing events without accesses does not rebuild")
  {
    exec.push_transition(action<IndependentAction>(1, MemOpType::Write, page0 + 8, 4));
    exec.push_transition(action<IndependentAction>(2));
    detector.check(exec);
    REQUIRE(detector.get_applied_count() == 1);

    exec.remove_last_event();
    exec.push_transition(action<IndependentAction>(3));
    exec.push_transition(action<IndependentAction>(3, MemOpType::Write, page0 + 64, 4));
    REQUIRE_NOTHROW(detector.check(exec));
    REQUIRE(detector.get_rebuild_count() == 0);
    REQUIRE(detector.get_applied_count() == 2);
  }
}
