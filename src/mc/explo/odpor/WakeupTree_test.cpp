/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/udpor/udpor_tests_private.hpp"
#include "src/xbt/utils/iter/LazyPowerset.hpp"

using namespace simgrid::mc;
using namespace simgrid::mc::odpor;
using namespace simgrid::mc::udpor;

static void test_tree_iterator(const WakeupTree& tree, const std::vector<PartialExecution>& expected)
{
  auto tree_iter = tree.begin();
  for (auto expected_iter = expected.begin(); expected_iter != expected.end(); ++expected_iter, ++tree_iter) {
    REQUIRE(tree_iter != tree.end());
    REQUIRE((*tree_iter)->get_sequence() == *expected_iter);
  }
}

TEST_CASE("simgrid::mc::odpor::WakeupTree: Testing Insertion for Empty Executions")
{
  SECTION("Following an execution")
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

    WakeupTree tree;

    SECTION("Attempting to insert the empty sequence into an empty tree should have no effect")
    {
      tree.insert(Execution(), {});
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{}});
    }

    // First, we initialize the tree to how it looked prior
    // to the insertion of the race
    tree.insert(Execution(), {a0});

    // Then, after insertion, we ensure that the node was
    // indeed added to the tree
    tree.insert(Execution(), {a1, a3});
    test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a0}, PartialExecution{a1, a3},
                                                           PartialExecution{a1}, PartialExecution{}});

    SECTION("Attempting to re-insert the same EXACT sequence should have no effect")
    {
      tree.insert(Execution(), {a1, a3});
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a0}, PartialExecution{a1, a3},
                                                             PartialExecution{a1}, PartialExecution{}});
    }

    SECTION("Attempting to re-insert an equivalent sequence should have no effect")
    {
      // a3 and a1 are interchangeable since `a1` is independent with everything.
      // Since we found an equivalent sequence that is a leaf, nothing should result
      tree.insert(Execution(), {a3, a1});
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a0}, PartialExecution{a1, a3},
                                                             PartialExecution{a1}, PartialExecution{}});
    }

    SECTION("Attempting to insert the empty sequence should have no effect")
    {
      tree.insert(Execution(), {});
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a0}, PartialExecution{a1, a3},
                                                             PartialExecution{a1}, PartialExecution{}});
    }

    SECTION("Inserting an extension should create a branch point")
    {
      // `a1.a2` shares the same `a1` prefix as `a1.a3`. Thus, the tree
      // should now look as follows:
      //
      //                 {}
      //               /    /
      //             a0     a1
      //                   /   /
      //                  a3   a2
      //
      // Recall that new nodes (in this case the one with
      // action `a2`) are added such that they are "greater than" (under
      // the tree's `<` relation) all those that exist under the given parent
      tree.insert(Execution(), {a1, a2});
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a0}, PartialExecution{a1, a3},
                                                             PartialExecution{a1, a2}, PartialExecution{a1},
                                                             PartialExecution{}});
    }
  }

  SECTION("Performing Arbitrary Insertions")
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

    WakeupTree tree;

    SECTION("Attempting to insert the empty sequence into an empty tree should have no effect")
    {
      tree.insert(Execution(), {});
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{}});
    }

    SECTION("Attempting to re-insert the same sequence multiple times should have no extra effect")
    {
      tree.insert(Execution(), {a4});
      tree.insert(Execution(), {a4});
      tree.insert(Execution(), {a4});
      REQUIRE(tree.get_num_nodes() == 2);
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a4}, PartialExecution{}});
    }

    SECTION("Attempting to insert an independent sequence same should have no extra effect")
    {
      // a4 and a1 are independent actions. Intuitively, then, we need only
      // search one ordering of the two actions. The wakeup tree handles
      // this by computing the `~` relation. The relation itself determines
      // whether the `a1` is an initial of `a3`, which it is not. It then
      // checks whether `a1` is independent with everything in the sequence
      // (in this case, consisting only of `a1`) which IS true. Since `a4`
      // is already a leaf node of the tree, it suffices to only have `a4`
      // in this tree to guide ODPOR
      tree.insert(Execution(), {a4});
      tree.insert(Execution(), {a1});
      REQUIRE(tree.get_num_nodes() == 2);
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a4}, PartialExecution{}});
    }

    SECTION(
        "Attempting to insert a progression of executions should have no extra effect when the first process is a leaf")
    {
      // All progressions starting with `a0` are effectively already accounted
      // for by inserting `a0` since we `a0` "can always be made to look like"
      // (viz. the `~` relation) `a0.*` where `*` is some sequence of actions
      tree.insert(Execution(), {a0});
      tree.insert(Execution(), {a0, a3});
      tree.insert(Execution(), {a0, a3, a2});
      tree.insert(Execution(), {a0, a3, a2, a4});
      tree.insert(Execution(), {a0, a3, a2, a4});
      REQUIRE(tree.get_num_nodes() == 2);
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a0}, PartialExecution{}});
    }

    SECTION("Stress test with multiple branch points: `~_E` with different looking sequences")
    {
      tree.insert(Execution(), {a0});
      tree.insert(Execution(), {a2, a0});
      tree.insert(Execution(), {a2, a3});
      tree.insert(Execution(), {a2, a5});
      REQUIRE(tree.get_num_nodes() == 6);
      test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a0}, PartialExecution{a2, a0},
                                                             PartialExecution{a2, a3}, PartialExecution{a2, a5},
                                                             PartialExecution{a2}, PartialExecution{}});
      SECTION("Adding more stress")
      {
        // In this case, `a2` and `a1` can be interchanged with each other.
        // Thus `a2.a1 == a1.a2`. Since there is already an interior node
        // containing `a2`, we attempt to add the what remains (viz. `a1`) to the
        // series. HOWEVER: we notice that `a2.a5` is "eventually equivalent to"
        // (that is `~` with) `a1.a2` since `a2` is an initial of the latter and
        // `a1` and `a5` are independent of each other
        tree.insert(Execution(), {a1, a2});
        REQUIRE(tree.get_num_nodes() == 6);
        test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a0}, PartialExecution{a2, a0},
                                                               PartialExecution{a2, a3}, PartialExecution{a2, a5},
                                                               PartialExecution{a2}, PartialExecution{}});

        tree.insert(Execution(), {a3, a0});
        REQUIRE(tree.get_num_nodes() == 8);
        test_tree_iterator(tree, std::vector<PartialExecution>{PartialExecution{a0}, PartialExecution{a2, a0},
                                                               PartialExecution{a2, a3}, PartialExecution{a2, a5},
                                                               PartialExecution{a2}, PartialExecution{a3, a0},
                                                               PartialExecution{a3}, PartialExecution{}});
      }
    }
  }
}

TEST_CASE("simgrid::mc::odpor::WakeupTree: Testing Subtree Rooting")
{
  const auto a0 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 3);
  const auto a1 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 4);
  const auto a2 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1);
  const auto a3 = std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 4);
  const auto a4 = std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 1);
  const auto a5 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 4);

  Execution execution;
  execution.push_transition(a0);
  execution.push_transition(a1);
  execution.push_transition(a2);
  execution.push_transition(a3);

  REQUIRE(execution.get_racing_events_of(2) == std::unordered_set<Execution::EventHandle>{0});
  REQUIRE(execution.get_racing_events_of(3) == std::unordered_set<Execution::EventHandle>{0});

  // The wakeup tree should

  WakeupTree tree;
}