/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/udpor/udpor_tests_private.hpp"

using namespace simgrid::mc;
using namespace simgrid::mc::odpor;
using namespace simgrid::mc::udpor;

TEST_CASE("simgrid::mc::odpor::WakeupTree: Testing Insertion")
{
  // We imagine the following completed execution `E`
  // consisting of executing actions a0-a3. Execution
  // `E` cis the first such maximal execution explored
  // by ODPOR, which implies that a) all sleep sets are
  // empty and b) all wakeup trees (i.e. for each prefix) consist of the root
  // node with a single leaf containing the action
  // taken, save for the wakeup tree of the execution itself
  // which is empty

  // We first notice that there's a reversible race between
  // events 0 and 3.

  const auto a0 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);
  const auto a1 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 4);
  const auto a2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
  const auto a3 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);

  Execution execution;
  execution.push_transition(a0);
  execution.push_transition(a1);
  execution.push_transition(a2);
  execution.push_transition(a3);

  REQUIRE(execution.get_racing_events_of(2) == std::unordered_set<Execution::EventHandle>{0});
  REQUIRE(execution.get_racing_events_of(3) == std::unordered_set<Execution::EventHandle>{0});

  // First, we initialize the tree to how it looked prior
  // to the insertion of the race
  WakeupTree tree;
  tree.insert(Execution(), {a0});

  // Then, after insertion, we ensure that the node was
  // indeed added to the tree

  tree.insert(Execution(), {a1, a3});

  WakeupTreeIterator iter = tree.begin();
  REQUIRE((*iter)->get_sequence() == PartialExecution{a0});

  ++iter;
  REQUIRE(iter != tree.end());
  REQUIRE((*iter)->get_sequence() == PartialExecution{a1, a3});

  ++iter;
  REQUIRE(iter != tree.end());
  REQUIRE((*iter)->get_sequence() == PartialExecution{});

  ++iter;
  REQUIRE(iter == tree.end());

  SECTION("Attempting to re-insert the same sequence should have no effect")
  {
    tree.insert(Execution(), {a1, a3});
    iter = tree.begin();

    REQUIRE((*iter)->get_sequence() == PartialExecution{a0});

    ++iter;
    REQUIRE(iter != tree.end());
    REQUIRE((*iter)->get_sequence() == PartialExecution{a1, a3});

    ++iter;
    REQUIRE(iter != tree.end());
    REQUIRE((*iter)->get_sequence() == PartialExecution{});

    ++iter;
    REQUIRE(iter == tree.end());
  }

  SECTION("Inserting an extension")
  {
    tree.insert(Execution(), {a1, a2});
    iter = tree.begin();
    REQUIRE((*iter)->get_sequence() == PartialExecution{a0});

    ++iter;
    REQUIRE(iter != tree.end());
    REQUIRE((*iter)->get_sequence() == PartialExecution{a1, a3});

    ++iter;
    REQUIRE(iter != tree.end());
    REQUIRE((*iter)->get_sequence() == PartialExecution{a1, a2});

    ++iter;
    REQUIRE(iter != tree.end());
    REQUIRE((*iter)->get_sequence() == PartialExecution{});

    ++iter;
    REQUIRE(iter == tree.end());
  }
}