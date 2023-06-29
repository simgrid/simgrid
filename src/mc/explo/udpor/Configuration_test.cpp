/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/maximal_subsets_iterator.hpp"
#include "src/mc/explo/udpor/udpor_tests_private.hpp"

#include <unordered_map>

using namespace simgrid::mc;
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
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(0));
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(1));
  UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(2));
  UnfoldingEvent e4(EventSet({&e3}), std::make_shared<IndependentAction>(3));
  UnfoldingEvent e5(EventSet({&e3}), std::make_shared<IndependentAction>(4));

  SECTION("Creating a configuration without events")
  {
    Configuration C;
    REQUIRE(C.get_events().empty());
    REQUIRE(C.get_latest_event() == nullptr);
  }

  SECTION("Creating a configuration with events (test violation of being causally closed)")
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
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(0));
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(1));
  UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(2));
  UnfoldingEvent e4(EventSet({&e2}), std::make_shared<IndependentAction>(3));

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

TEST_CASE("simgrid::mc::udpor::Configuration: Topological Sort Order")
{
  // The following tests concern the given event structure:
  //               e1
  //              /
  //            e2
  //           /
  //          e3
  //         /
  //        e4
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(1));
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(2));
  UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(3));
  UnfoldingEvent e4(EventSet({&e3}), std::make_shared<IndependentAction>(4));

  SECTION("Topological ordering for entire set")
  {
    Configuration C{&e1, &e2, &e3, &e4};
    REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3, &e4});
    REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() ==
            std::vector<const UnfoldingEvent*>{&e4, &e3, &e2, &e1});
  }

  SECTION("Topological ordering for subsets")
  {
    SECTION("No elements")
    {
      Configuration C;
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() == std::vector<const UnfoldingEvent*>{});
    }

    SECTION("e1 only")
    {
      Configuration C{&e1};
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() == std::vector<const UnfoldingEvent*>{&e1});
    }

    SECTION("e1 and e2 only")
    {
      Configuration C{&e1, &e2};
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() == std::vector<const UnfoldingEvent*>{&e2, &e1});
    }

    SECTION("e1, e2, and e3 only")
    {
      Configuration C{&e1, &e2, &e3};
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() ==
              std::vector<const UnfoldingEvent*>{&e3, &e2, &e1});
    }
  }
}

TEST_CASE("simgrid::mc::udpor::Configuration: Topological Sort Order More Complicated")
{
  // The following tests concern the given event structure:
  //                e1
  //              /
  //            e2
  //           /
  //          e3
  //         /  /
  //        e4   e6
  //        /
  //       e5
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(1));
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(2));
  UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(3));
  UnfoldingEvent e4(EventSet({&e3}), std::make_shared<IndependentAction>(4));
  UnfoldingEvent e5(EventSet({&e4}), std::make_shared<IndependentAction>(5));
  UnfoldingEvent e6(EventSet({&e3}), std::make_shared<IndependentAction>(6));

  SECTION("Topological ordering for subsets")
  {
    SECTION("No elements")
    {
      Configuration C;
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() == std::vector<const UnfoldingEvent*>{});
    }

    SECTION("e1 only")
    {
      Configuration C{&e1};
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() == std::vector<const UnfoldingEvent*>{&e1});
    }

    SECTION("e1 and e2 only")
    {
      Configuration C{&e1, &e2};
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() == std::vector<const UnfoldingEvent*>{&e2, &e1});
    }

    SECTION("e1, e2, and e3 only")
    {
      Configuration C{&e1, &e2, &e3};
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() ==
              std::vector<const UnfoldingEvent*>{&e3, &e2, &e1});
    }

    SECTION("e1, e2, e3, and e6 only")
    {
      Configuration C{&e1, &e2, &e3, &e6};
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3, &e6});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() ==
              std::vector<const UnfoldingEvent*>{&e6, &e3, &e2, &e1});
    }

    SECTION("e1, e2, e3, and e4 only")
    {
      Configuration C{&e1, &e2, &e3, &e4};
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3, &e4});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() ==
              std::vector<const UnfoldingEvent*>{&e4, &e3, &e2, &e1});
    }

    SECTION("e1, e2, e3, e4, and e5 only")
    {
      Configuration C{&e1, &e2, &e3, &e4, &e5};
      REQUIRE(C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3, &e4, &e5});
      REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() ==
              std::vector<const UnfoldingEvent*>{&e5, &e4, &e3, &e2, &e1});
    }

    SECTION("e1, e2, e3, e4 and e6 only")
    {
      // In this case, e4 and e6 are interchangeable. Hence, we have to check
      // if the sorting gives us *any* of the combinations
      Configuration C{&e1, &e2, &e3, &e4, &e6};
      REQUIRE((C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3, &e4, &e6} ||
               C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3, &e6, &e4}));
      REQUIRE((C.get_topologically_sorted_events_of_reverse_graph() ==
                   std::vector<const UnfoldingEvent*>{&e6, &e4, &e3, &e2, &e1} ||
               C.get_topologically_sorted_events_of_reverse_graph() ==
                   std::vector<const UnfoldingEvent*>{&e4, &e6, &e3, &e2, &e1}));
    }

    SECTION("Topological ordering for entire set")
    {
      // In this case, e4/e5 are both interchangeable with e6. Hence, again we have to check
      // if the sorting gives us *any* of the combinations
      Configuration C{&e1, &e2, &e3, &e4, &e5, &e6};
      REQUIRE(
          (C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3, &e4, &e5, &e6} ||
           C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3, &e4, &e6, &e5} ||
           C.get_topologically_sorted_events() == std::vector<const UnfoldingEvent*>{&e1, &e2, &e3, &e6, &e4, &e5}));
      REQUIRE((C.get_topologically_sorted_events_of_reverse_graph() ==
                   std::vector<const UnfoldingEvent*>{&e6, &e5, &e4, &e3, &e2, &e1} ||
               C.get_topologically_sorted_events_of_reverse_graph() ==
                   std::vector<const UnfoldingEvent*>{&e5, &e6, &e4, &e3, &e2, &e1} ||
               C.get_topologically_sorted_events_of_reverse_graph() ==
                   std::vector<const UnfoldingEvent*>{&e5, &e4, &e6, &e3, &e2, &e1}));
    }
  }
}

TEST_CASE("simgrid::mc::udpor::Configuration: Topological Sort Order Very Complicated")
{
  // The following tests concern the given event structure:
  //                e1
  //              /   /
  //            e2    e8
  //           /  /    /  /
  //          e3   /   /    /
  //         /  /    /      e11
  //        e4  e6   e7
  //        /   /  /   /
  //       e5   e9    e10
  //        /   /     /
  //         /  /   /
  //         [   e12    ]
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(1));
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(2));
  UnfoldingEvent e8(EventSet({&e1}), std::make_shared<IndependentAction>(3));
  UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(4));
  UnfoldingEvent e4(EventSet({&e3}), std::make_shared<IndependentAction>(5));
  UnfoldingEvent e5(EventSet({&e4}), std::make_shared<IndependentAction>(6));
  UnfoldingEvent e6(EventSet({&e4}), std::make_shared<IndependentAction>(7));
  UnfoldingEvent e7(EventSet({&e2, &e8}), std::make_shared<IndependentAction>(8));
  UnfoldingEvent e9(EventSet({&e6, &e7}), std::make_shared<IndependentAction>(9));
  UnfoldingEvent e10(EventSet({&e7}), std::make_shared<IndependentAction>(10));
  UnfoldingEvent e11(EventSet({&e8}), std::make_shared<IndependentAction>(11));
  UnfoldingEvent e12(EventSet({&e5, &e9, &e10}), std::make_shared<IndependentAction>(12));
  Configuration C{&e1, &e2, &e3, &e4, &e5, &e6, &e7, &e8, &e9, &e10, &e11, &e12};

  SECTION("Test every combination of the maximal configuration (forward graph)")
  {
    // To test this, we ensure that for the `i`th event
    // in `ordered_events`, each event in `ordered_events[0...<i]
    // is contained in the history of `ordered_events[i]`.
    EventSet events_seen;
    const auto ordered_events = C.get_topologically_sorted_events();

    std::for_each(ordered_events.begin(), ordered_events.end(), [&events_seen](const UnfoldingEvent* e) {
      History history(e);
      for (const auto* e_hist : history) {
        // In this demo, we want to make sure that
        // we don't mark not yet seeing `e` as an error.
        // The history of `e` traverses `e` itself. All
        // other events in e's history are included in
        if (e_hist == e)
          continue;

        // If this event has not been "seen" before,
        // this implies that event `e` appears earlier
        // in the list than one of its dependencies
        REQUIRE(events_seen.contains(e_hist));
      }
      events_seen.insert(e);
    });
  }

  SECTION("Test every combination of the maximal configuration (reverse graph)")
  {
    // To test this, we ensure that for the `i`th event
    // in `ordered_events`, no event in `ordered_events[0...<i]
    // is contained in the history of `ordered_events[i]`.
    EventSet events_seen;
    const auto ordered_events = C.get_topologically_sorted_events_of_reverse_graph();

    std::for_each(ordered_events.begin(), ordered_events.end(), [&events_seen](const UnfoldingEvent* e) {
      History history(e);

      for (const auto* e_hist : history) {
        // Unlike the test above, we DO want to ensure
        // that `e` itself ALSO isn't yet seen

        // If this event has been "seen" before,
        // this implies that event `e` appears later
        // in the list than one of its ancestors
        REQUIRE_FALSE(events_seen.contains(e_hist));
      }
      events_seen.insert(e);
    });
  }

  SECTION("Test that the topological ordering contains only the events of the configuration")
  {
    const EventSet events_seen = C.get_events();

    SECTION("Forward direction")
    {
      const auto ordered_event_set = EventSet(C.get_topologically_sorted_events());
      REQUIRE(events_seen == ordered_event_set);
    }

    SECTION("Reverse direction")
    {
      const auto ordered_event_set = EventSet(C.get_topologically_sorted_events_of_reverse_graph());
      REQUIRE(events_seen == ordered_event_set);
    }
  }

  SECTION("Test that the topological ordering is equivalent to that of the configuration's events")
  {
    REQUIRE(C.get_topologically_sorted_events() == C.get_events().get_topological_ordering());
    REQUIRE(C.get_topologically_sorted_events_of_reverse_graph() ==
            C.get_events().get_topological_ordering_of_reverse_graph());
  }
}

TEST_CASE("simgrid::mc::udpor::maximal_subsets_iterator: Basic Testing of Maximal Subsets")
{
  // The following tests concern the given event structure:
  //                e1
  //              /   /
  //             e2   e5
  //            /     /
  //           e3    e6
  //           /     / /
  //          e4    e7 e8
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(0));
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(1));
  UnfoldingEvent e3(EventSet({&e2}), std::make_shared<IndependentAction>(2));
  UnfoldingEvent e4(EventSet({&e3}), std::make_shared<IndependentAction>(3));
  UnfoldingEvent e5(EventSet({&e1}), std::make_shared<IndependentAction>(4));
  UnfoldingEvent e6(EventSet({&e5}), std::make_shared<IndependentAction>(5));
  UnfoldingEvent e7(EventSet({&e6}), std::make_shared<IndependentAction>(6));
  UnfoldingEvent e8(EventSet({&e6}), std::make_shared<IndependentAction>(7));

  SECTION("Iteration over an empty configuration yields only the empty set")
  {
    Configuration C;
    maximal_subsets_iterator first(C);
    maximal_subsets_iterator last;

    REQUIRE(*first == EventSet());
    ++first;
    REQUIRE(first == last);
  }

  SECTION("Check counts of maximal event sets discovered")
  {
    std::unordered_map<int, int> maximal_subset_counts;

    Configuration C{&e1, &e2, &e3, &e4, &e5, &e6, &e7, &e8};
    maximal_subsets_iterator first(C);
    maximal_subsets_iterator last;

    for (; first != last; ++first) {
      maximal_subset_counts[(*first).size()]++;
    }

    // First, ensure that there are only sets of size 0, 1, 2, and 3
    CHECK(maximal_subset_counts.size() == 4);

    // The empty set should appear only once
    REQUIRE(maximal_subset_counts[0] == 1);

    // 8 is the number of nodes in the graph
    REQUIRE(maximal_subset_counts[1] == 8);

    // 13 = 3 * 4 (each of the left branch can combine with one in the right branch) + 1 (e7 + e8)
    REQUIRE(maximal_subset_counts[2] == 13);

    // e7 + e8 must be included, so that means we can combine from the left branch
    REQUIRE(maximal_subset_counts[3] == 3);
  }

  SECTION("Check counts of maximal event sets discovered with a filter")
  {
    std::unordered_map<int, int> maximal_subset_counts;

    Configuration C{&e1, &e2, &e3, &e4, &e5, &e6, &e7, &e8};

    SECTION("Filter with events part of initial maximal set")
    {
      EventSet interesting_bunch{&e2, &e4, &e7, &e8};

      maximal_subsets_iterator first(
          C, [&interesting_bunch](const UnfoldingEvent* e) { return interesting_bunch.contains(e); });
      maximal_subsets_iterator last;

      for (; first != last; ++first) {
        const auto& event_set = *first;
        // Only events in `interesting_bunch` can appear: thus no set
        // should include anything else other than `interesting_bunch`
        REQUIRE(event_set.is_subset_of(interesting_bunch));
        REQUIRE(event_set.is_maximal());
        maximal_subset_counts[event_set.size()]++;
      }

      // The empty set should (still) appear only once
      REQUIRE(maximal_subset_counts[0] == 1);

      // 4 is the number of nodes in the `interesting_bunch`
      REQUIRE(maximal_subset_counts[1] == 4);

      // 5 = 2 * 2 (each of the left branch can combine with one in the right branch) + 1 (e7 + e8)
      REQUIRE(maximal_subset_counts[2] == 5);

      // e7 + e8 must be included, so that means we can combine from the left branch (only e2 and e4)
      REQUIRE(maximal_subset_counts[3] == 2);

      // There are no subsets of size 4 (or higher, but that
      // is tested by asserting each maximal set traversed is a subset)
      REQUIRE(maximal_subset_counts[4] == 0);
    }

    SECTION("Filter with interesting subset not initially part of the maximal set")
    {
      EventSet interesting_bunch{&e3, &e5, &e6};

      maximal_subsets_iterator first(
          C, [&interesting_bunch](const UnfoldingEvent* e) { return interesting_bunch.contains(e); });
      maximal_subsets_iterator last;

      for (; first != last; ++first) {
        const auto& event_set = *first;
        // Only events in `interesting_bunch` can appear: thus no set
        // should include anything else other than `interesting_bunch`
        REQUIRE(event_set.is_subset_of(interesting_bunch));
        REQUIRE(event_set.is_maximal());
        maximal_subset_counts[event_set.size()]++;
      }

      // The empty set should (still) appear only once
      REQUIRE(maximal_subset_counts[0] == 1);

      // 3 is the number of nodes in the `interesting_bunch`
      REQUIRE(maximal_subset_counts[1] == 3);

      // 2 = e3, e5 and e3, e6
      REQUIRE(maximal_subset_counts[2] == 2);

      // There are no subsets of size 3 (or higher, but that
      // is tested by asserting each maximal set traversed is a subset)
      REQUIRE(maximal_subset_counts[3] == 0);
    }
  }
}

TEST_CASE("simgrid::mc::udpor::maximal_subsets_iterator: Stress Test for Maximal Subsets Iteration")
{
  // The following tests concern the given event structure:
  //                              e1
  //                            /   /
  //                          e2    e3
  //                          / /   /  /
  //               +------* e4 *e5 e6  e7
  //               |        /   ///   /  /
  //               |       e8   e9    e10
  //               |      /  /   /\      /
  //               |   e11 e12 e13 e14   e15
  //               |   /      / / /   /  /
  //               +-> e16     e17     e18
  UnfoldingEvent e1(EventSet(), std::make_shared<IndependentAction>(1));
  UnfoldingEvent e2(EventSet({&e1}), std::make_shared<IndependentAction>(2));
  UnfoldingEvent e3(EventSet({&e1}), std::make_shared<IndependentAction>(3));
  UnfoldingEvent e4(EventSet({&e2}), std::make_shared<IndependentAction>(4));
  UnfoldingEvent e5(EventSet({&e2}), std::make_shared<IndependentAction>(5));
  UnfoldingEvent e6(EventSet({&e3}), std::make_shared<IndependentAction>(6));
  UnfoldingEvent e7(EventSet({&e3}), std::make_shared<IndependentAction>(7));
  UnfoldingEvent e8(EventSet({&e4}), std::make_shared<IndependentAction>(8));
  UnfoldingEvent e9(EventSet({&e4, &e5, &e6}), std::make_shared<IndependentAction>(9));
  UnfoldingEvent e10(EventSet({&e6, &e7}), std::make_shared<IndependentAction>(10));
  UnfoldingEvent e11(EventSet({&e8}), std::make_shared<IndependentAction>(11));
  UnfoldingEvent e12(EventSet({&e8}), std::make_shared<IndependentAction>(12));
  UnfoldingEvent e13(EventSet({&e9}), std::make_shared<IndependentAction>(13));
  UnfoldingEvent e14(EventSet({&e9}), std::make_shared<IndependentAction>(14));
  UnfoldingEvent e15(EventSet({&e10}), std::make_shared<IndependentAction>(15));
  UnfoldingEvent e16(EventSet({&e5, &e11}), std::make_shared<IndependentAction>(16));
  UnfoldingEvent e17(EventSet({&e12, &e13, &e14}), std::make_shared<IndependentAction>(17));
  UnfoldingEvent e18(EventSet({&e14, &e15}), std::make_shared<IndependentAction>(18));
  Configuration C{&e1, &e2, &e3, &e4, &e5, &e6, &e7, &e8, &e9, &e10, &e11, &e12, &e13, &e14, &e15, &e16, &e17, &e18};

  SECTION("Every subset iterated over is maximal")
  {
    maximal_subsets_iterator first(C);
    maximal_subsets_iterator last;

    // Make sure we actually have something to iterate over
    REQUIRE(first != last);

    for (; first != last; ++first) {
      REQUIRE((*first).size() <= C.get_events().size());
      REQUIRE((*first).is_maximal());
    }
  }

  SECTION("Test that the maximal set ordering is equivalent to that of the configuration's events")
  {
    maximal_subsets_iterator first_config(C);
    maximal_subsets_iterator first_events(C.get_events());
    maximal_subsets_iterator last;

    // Make sure we actually have something to iterate over
    REQUIRE(first_config != last);
    REQUIRE(first_config == first_events);
    REQUIRE(first_events != last);

    for (; first_config != last; ++first_config, ++first_events) {
      // first_events and first_config should always be at the same location
      REQUIRE(first_events != last);
      const auto& first_config_set = *first_config;
      const auto& first_events_set = *first_events;

      REQUIRE(first_config_set.size() <= C.get_events().size());
      REQUIRE(first_config_set.is_maximal());
      REQUIRE(first_events_set == first_config_set);
    }

    // Iteration with events directly should now also be finished
    REQUIRE(first_events == last);
  }
}

TEST_CASE("simgrid::mc::udpor::Configuration: Latest Transitions")
{
  // The following tests concern the given event structure (labeled as "event(actor)")
  //                  e1(1)
  //                 /     /
  //              e2(1)   e3(2)
  //              /    //     /
  //            e4(3) e5(2)  e6(1)
  //                  /   /
  //               e7(1) e8(1)
  const auto t1 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
  const auto t2 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
  const auto t3 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
  const auto t4 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 3);
  const auto t5 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 2);
  const auto t6 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
  const auto t7 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);
  const auto t8 = std::make_shared<IndependentAction>(Transition::Type::UNKNOWN, 1);

  const UnfoldingEvent e1(EventSet(), t1);
  const UnfoldingEvent e2(EventSet({&e1}), t2);
  const UnfoldingEvent e3(EventSet({&e1}), t3);
  const UnfoldingEvent e4(EventSet({&e2}), t4);
  const UnfoldingEvent e5(EventSet({&e2, &e3}), t5);
  const UnfoldingEvent e6(EventSet({&e3}), t6);
  const UnfoldingEvent e7(EventSet({&e5}), t7);
  const UnfoldingEvent e8(EventSet({&e5}), t8);

  SECTION("Test that the latest events are correct on initialization")
  {
    SECTION("Empty configuration has no events")
    {
      Configuration C;
      REQUIRE_FALSE(C.get_latest_event_of(1).has_value());
      REQUIRE_FALSE(C.get_latest_event_of(2).has_value());
      REQUIRE_FALSE(C.get_latest_event_of(3).has_value());

      REQUIRE_FALSE(C.get_latest_action_of(1).has_value());
      REQUIRE_FALSE(C.get_latest_action_of(2).has_value());
      REQUIRE_FALSE(C.get_latest_action_of(3).has_value());
    }

    SECTION("Missing two actors")
    {
      Configuration C{&e1};
      REQUIRE(C.get_latest_event_of(1).has_value());
      REQUIRE(C.get_latest_event_of(1).value() == &e1);

      REQUIRE_FALSE(C.get_latest_event_of(2).has_value());
      REQUIRE_FALSE(C.get_latest_event_of(3).has_value());

      REQUIRE(C.get_latest_action_of(1).has_value());
      REQUIRE(C.get_latest_action_of(1).value() == t1.get());

      REQUIRE_FALSE(C.get_latest_action_of(2).has_value());
      REQUIRE_FALSE(C.get_latest_action_of(3).has_value());
    }

    SECTION("Two events with one actor yields the latest event")
    {
      Configuration C{&e1, &e2};
      REQUIRE(C.get_latest_event_of(1).has_value());
      REQUIRE(C.get_latest_event_of(1).value() == &e2);

      REQUIRE_FALSE(C.get_latest_event_of(2).has_value());
      REQUIRE_FALSE(C.get_latest_event_of(3).has_value());

      REQUIRE(C.get_latest_action_of(1).has_value());
      REQUIRE(C.get_latest_action_of(1).value() == t2.get());

      REQUIRE_FALSE(C.get_latest_action_of(2).has_value());
      REQUIRE_FALSE(C.get_latest_action_of(3).has_value());
    }

    SECTION("Two events with two actors")
    {
      Configuration C{&e1, &e3};
      REQUIRE(C.get_latest_event_of(1).has_value());
      REQUIRE(C.get_latest_event_of(1).value() == &e1);

      REQUIRE(C.get_latest_event_of(2).has_value());
      REQUIRE(C.get_latest_event_of(2).value() == &e3);

      REQUIRE_FALSE(C.get_latest_event_of(3).has_value());

      REQUIRE(C.get_latest_action_of(1).has_value());
      REQUIRE(C.get_latest_action_of(1).value() == t1.get());

      REQUIRE(C.get_latest_action_of(2).has_value());
      REQUIRE(C.get_latest_action_of(2).value() == t3.get());

      REQUIRE_FALSE(C.get_latest_action_of(3).has_value());
    }

    SECTION("Three different actors actors")
    {
      Configuration C{&e1, &e2, &e3, &e4, &e5};
      REQUIRE(C.get_latest_event_of(1).has_value());
      REQUIRE(C.get_latest_event_of(1).value() == &e2);

      REQUIRE(C.get_latest_event_of(2).has_value());
      REQUIRE(C.get_latest_event_of(2).value() == &e5);

      REQUIRE(C.get_latest_event_of(3).has_value());
      REQUIRE(C.get_latest_event_of(3).value() == &e4);

      REQUIRE(C.get_latest_action_of(1).has_value());
      REQUIRE(C.get_latest_action_of(1).value() == t2.get());

      REQUIRE(C.get_latest_action_of(2).has_value());
      REQUIRE(C.get_latest_action_of(2).value() == t5.get());

      REQUIRE(C.get_latest_action_of(3).has_value());
      REQUIRE(C.get_latest_action_of(3).value() == t4.get());
    }
  }

  SECTION("Test that the latest events are correct when adding new events")
  {
    Configuration C;
    REQUIRE_FALSE(C.get_latest_event_of(1).has_value());
    REQUIRE_FALSE(C.get_latest_event_of(2).has_value());
    REQUIRE_FALSE(C.get_latest_event_of(3).has_value());
    REQUIRE_FALSE(C.get_latest_action_of(1).has_value());
    REQUIRE_FALSE(C.get_latest_action_of(2).has_value());
    REQUIRE_FALSE(C.get_latest_action_of(3).has_value());

    C.add_event(&e1);
    REQUIRE(C.get_latest_event_of(1).has_value());
    REQUIRE(C.get_latest_event_of(1).value() == &e1);
    REQUIRE_FALSE(C.get_latest_event_of(2).has_value());
    REQUIRE_FALSE(C.get_latest_event_of(3).has_value());
    REQUIRE(C.get_latest_action_of(1).has_value());
    REQUIRE(C.get_latest_action_of(1).value() == t1.get());
    REQUIRE_FALSE(C.get_latest_action_of(2).has_value());
    REQUIRE_FALSE(C.get_latest_action_of(3).has_value());

    C.add_event(&e2);
    REQUIRE(C.get_latest_event_of(1).has_value());
    REQUIRE(C.get_latest_event_of(1).value() == &e2);
    REQUIRE_FALSE(C.get_latest_event_of(2).has_value());
    REQUIRE_FALSE(C.get_latest_event_of(3).has_value());
    REQUIRE(C.get_latest_action_of(1).has_value());
    REQUIRE(C.get_latest_action_of(1).value() == t2.get());
    REQUIRE_FALSE(C.get_latest_action_of(2).has_value());
    REQUIRE_FALSE(C.get_latest_action_of(3).has_value());

    C.add_event(&e3);
    REQUIRE(C.get_latest_event_of(1).has_value());
    REQUIRE(C.get_latest_event_of(1).value() == &e2);
    REQUIRE(C.get_latest_event_of(2).has_value());
    REQUIRE(C.get_latest_event_of(2).value() == &e3);
    REQUIRE_FALSE(C.get_latest_event_of(3).has_value());
    REQUIRE(C.get_latest_action_of(1).has_value());
    REQUIRE(C.get_latest_action_of(1).value() == t2.get());
    REQUIRE(C.get_latest_action_of(2).has_value());
    REQUIRE(C.get_latest_action_of(2).value() == t3.get());
    REQUIRE_FALSE(C.get_latest_action_of(3).has_value());

    C.add_event(&e4);
    REQUIRE(C.get_latest_event_of(1).has_value());
    REQUIRE(C.get_latest_event_of(1).value() == &e2);
    REQUIRE(C.get_latest_event_of(2).has_value());
    REQUIRE(C.get_latest_event_of(2).value() == &e3);
    REQUIRE(C.get_latest_event_of(3).has_value());
    REQUIRE(C.get_latest_event_of(3).value() == &e4);
    REQUIRE(C.get_latest_action_of(1).has_value());
    REQUIRE(C.get_latest_action_of(1).value() == t2.get());
    REQUIRE(C.get_latest_action_of(2).has_value());
    REQUIRE(C.get_latest_action_of(2).value() == t3.get());
    REQUIRE(C.get_latest_action_of(3).has_value());
    REQUIRE(C.get_latest_action_of(3).value() == t4.get());

    C.add_event(&e5);
    REQUIRE(C.get_latest_event_of(1).has_value());
    REQUIRE(C.get_latest_event_of(1).value() == &e2);
    REQUIRE(C.get_latest_event_of(2).has_value());
    REQUIRE(C.get_latest_event_of(2).value() == &e5);
    REQUIRE(C.get_latest_event_of(3).has_value());
    REQUIRE(C.get_latest_event_of(3).value() == &e4);
    REQUIRE(C.get_latest_action_of(1).has_value());
    REQUIRE(C.get_latest_action_of(1).value() == t2.get());
    REQUIRE(C.get_latest_action_of(2).has_value());
    REQUIRE(C.get_latest_action_of(2).value() == t5.get());
    REQUIRE(C.get_latest_action_of(3).has_value());
    REQUIRE(C.get_latest_action_of(3).value() == t4.get());
  }
}

TEST_CASE("simgrid::mc::udpor::Configuration: Computing Full Alternatives in Reader/Writer Example")
{
  // The following tests concern the given event structure that is given as
  // an example in figure 1 of the original UDPOR paper.
  //                  e0
  //              /  /   /
  //            e1   e4   e7
  //           /     /  //   /
  //         /  /   e5  e8   e9
  //        e2 e3   /        /
  //               e6       e10
  //
  // Theses tests walk through exactly the configurations and sets of `D` that
  // UDPOR COULD encounter as it walks through the unfolding. Note that
  // if there are multiple alternatives to any given configuration, UDPOR can
  // continue searching any one of them. The sequence assumes UDPOR first picks `e1`,
  // then `e4`, and then `e7`
  Unfolding U;

  auto e0 = std::make_unique<UnfoldingEvent>(
      EventSet(), std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 0));
  const auto* e0_handle = e0.get();

  auto e1        = std::make_unique<UnfoldingEvent>(EventSet({e0_handle}),
                                             std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 0));
  const auto* e1_handle = e1.get();

  auto e2 = std::make_unique<UnfoldingEvent>(
      EventSet({e1_handle}), std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1));
  const auto* e2_handle = e2.get();

  auto e3 = std::make_unique<UnfoldingEvent>(
      EventSet({e1_handle}), std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2));
  const auto* e3_handle = e3.get();

  auto e4 = std::make_unique<UnfoldingEvent>(
      EventSet({e0_handle}), std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1));
  const auto* e4_handle = e4.get();

  auto e5        = std::make_unique<UnfoldingEvent>(EventSet({e4_handle}),
                                             std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 0));
  const auto* e5_handle = e5.get();

  auto e6 = std::make_unique<UnfoldingEvent>(
      EventSet({e5_handle}), std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2));
  const auto* e6_handle = e6.get();

  auto e7 = std::make_unique<UnfoldingEvent>(
      EventSet({e0_handle}), std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 2));
  const auto* e7_handle = e7.get();

  auto e8        = std::make_unique<UnfoldingEvent>(EventSet({e4_handle, e7_handle}),
                                             std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 0));
  const auto* e8_handle = e8.get();

  auto e9        = std::make_unique<UnfoldingEvent>(EventSet({e7_handle}),
                                             std::make_shared<DependentAction>(Transition::Type::UNKNOWN, 0));
  const auto* e9_handle = e9.get();

  auto e10 = std::make_unique<UnfoldingEvent>(
      EventSet({e9_handle}), std::make_shared<ConditionallyDependentAction>(Transition::Type::UNKNOWN, 1));
  const auto* e10_handle = e10.get();

  SECTION("Alternative computation call 1")
  {
    // During the first call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                  e0
    //              /  /   /
    //            e1   e4   e7
    //           /
    //         /  /
    //        e2 e3
    //
    // C := {e0, e1, e2} and `Explore(C, D, A)` picked `e3`
    // (since en(C') where C' := {e0, e1, e2, e3} is empty
    // [so UDPOR will simply return when C' is reached])
    //
    // Thus the computation is (since D is empty at first)
    //
    // Alt(C, D + {e}) --> Alt({e0, e1, e2}, {e3})
    //
    // where U is given above. There are no alternatives in
    // this case since `e4` and `e7` conflict with `e1` (so
    // they cannot be added to C to form a configuration)
    const Configuration C{e0_handle, e1_handle, e2_handle};
    const EventSet D_plus_e{e3_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e7));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE_FALSE(alternative.has_value());
  }

  SECTION("Alternative computation call 2")
  {
    // During the second call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                  e0
    //              /  /   /
    //            e1   e4   e7
    //           /
    //         /  /
    //        e2 e3
    //
    // C := {e0, e1} and `Explore(C, D, A)` picked `e2`.
    //
    // Thus the computation is (since D is still empty)
    //
    // Alt(C, D + {e}) --> Alt({e0, e1}, {e2})
    //
    // where U is given above. There are no alternatives in
    // this case since `e4` and `e7` conflict with `e1` (so
    // they cannot be added to C to form a configuration) and
    // e3 is NOT in conflict with either e0 or e1
    const Configuration C{e0_handle, e1_handle};
    const EventSet D_plus_e{e2_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e7));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE_FALSE(alternative.has_value());
  }

  SECTION("Alternative computation call 3")
  {
    // During the thrid call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                 e0
    //              /  /   /
    //            e1   e4   e7
    //           /
    //         /  /
    //        e2 e3
    //
    // C := {e0} and `Explore(C, D, A)` picked `e1`.
    //
    // Thus the computation is (since D is still empty)
    //
    // Alt(C, D + {e}) --> Alt({e0}, {e1})
    //
    // where U is given above. There are two alternatives in this case:
    // {e0, e4} and {e0, e7}. Either one would be a valid choice for
    // UDPOR, so we must check for the precense of either
    const Configuration C{e0_handle};
    const EventSet D_plus_e{e1_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e7));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE(alternative.has_value());

    // The first alternative that is found is the one that is chosen. Since
    // traversal over the elements of an unordered_set<> are not guaranteed,
    // both {e0, e4} and {e0, e7} are valid alternatives
    REQUIRE((alternative.value().get_events() == EventSet({e0_handle, e4_handle}) ||
             alternative.value().get_events() == EventSet({e0_handle, e7_handle})));
  }

  SECTION("Alternative computation call 4")
  {
    // During the fourth call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                  e0
    //              /  /   /
    //            e1   e4   e7
    //           /     /  //
    //         /  /   e5  e8
    //        e2 e3   /
    //               e6
    //
    // C := {e0, e4, e5} and `Explore(C, D, A)` picked `e6`
    // (since en(C') where C' := {e0, e4, e5, e6} is empty
    // [so UDPOR will simply return when C' is reached])
    //
    // Thus the computation is (since D is {e1})
    //
    // Alt(C, D + {e}) --> Alt({e0, e4, e5}, {e1, e6})
    //
    // where U is given above. There are no alternatives in this
    // case, since:
    //
    // 1.`e2/e3` are eliminated since their histories contain `e1`
    // 2. `e7/e8` are eliminated because they conflict with `e5`
    const Configuration C{e0_handle, e4_handle, e5_handle};
    const EventSet D_plus_e{e1_handle, e6_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e6));
    U.insert(std::move(e7));
    U.insert(std::move(e8));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE_FALSE(alternative.has_value());
  }

  SECTION("Alternative computation call 5")
  {
    // During the fifth call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                  e0
    //              /  /   /
    //            e1   e4   e7
    //           /     /  //
    //         /  /   e5  e8
    //        e2 e3   /
    //               e6
    //
    // C := {e0, e4} and `Explore(C, D, A)` picked `e5`
    // (since en(C') where C' := {e0, e4, e5, e6} is empty
    // [so UDPOR will simply return when C' is reached])
    //
    // Thus the computation is (since D is {e1})
    //
    // Alt(C, D + {e}) --> Alt({e0, e4}, {e1, e5})
    //
    // where U is given above. There are THREE alternatives in this case,
    // viz. {e0, e7}, {e0, e4, e7} and {e0, e4, e7, e8}.
    //
    // To continue the search, UDPOR computes J / C which in this
    // case gives {e7, e8}. Since `e8` is not in en(C), UDPOR will
    // choose `e7` next and add `e5` to `D`
    const Configuration C{e0_handle, e4_handle};
    const EventSet D_plus_e{e1_handle, e5_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e6));
    U.insert(std::move(e7));
    U.insert(std::move(e8));
    REQUIRE(U.size() == 8);

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE(alternative.has_value());
    REQUIRE((alternative.value().get_events() == EventSet({e0_handle, e7_handle}) ||
             alternative.value().get_events() == EventSet({e0_handle, e4_handle, e7_handle}) ||
             alternative.value().get_events() == EventSet({e0_handle, e4_handle, e7_handle, e8_handle})));
  }

  SECTION("Alternative computation call 6")
  {
    // During the sixth call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                 e0
    //              /  /   /
    //            e1   e4   e7
    //           /     /  //   /
    //         /  /   e5  e8   e9
    //        e2 e3   /
    //               e6
    //
    // C := {e0, e4, e7} and `Explore(C, D, A)` picked `e8`
    // (since en(C') where C' := {e0, e4, e7, e8} is empty
    // [so UDPOR will simply return when C' is reached])
    //
    // Thus the computation is (since D is {e1, e5} [see the last step])
    //
    // Alt(C, D + {e}) --> Alt({e0, e4, e7}, {e1, e5, e8})
    //
    // where U is given above. There are no alternatives in this case
    // since all `e9` conflicts with `e4` and all other events of `U`
    // are eliminated since their history intersects `D`
    const Configuration C{e0_handle, e4_handle, e7_handle};
    const EventSet D_plus_e{e1_handle, e5_handle, e8_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e6));
    U.insert(std::move(e7));
    U.insert(std::move(e8));
    U.insert(std::move(e9));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE_FALSE(alternative.has_value());
  }

  SECTION("Alternative computation call 7")
  {
    // During the seventh call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                 e0
    //              /  /   /
    //            e1   e4   e7
    //           /     /  //   /
    //         /  /   e5  e8   e9
    //        e2 e3   /
    //               e6
    //
    // C := {e0, e4} and `Explore(C, D, A)` picked `e7`
    //
    // Thus the computation is (since D is {e1, e5} [see call 5])
    //
    // Alt(C, D + {e}) --> Alt({e0, e4}, {e1, e5, e7})
    //
    // where U is given above. There are no alternatives again in this case
    // since all `e9` conflicts with `e4` and all other events of `U`
    // are eliminated since their history intersects `D`
    const Configuration C{e0_handle, e4_handle};
    const EventSet D_plus_e{e1_handle, e5_handle, e7_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e6));
    U.insert(std::move(e7));
    U.insert(std::move(e8));
    U.insert(std::move(e9));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE_FALSE(alternative.has_value());
  }

  SECTION("Alternative computation call 8")
  {
    // During the eigth call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                 e0
    //              /  /   /
    //            e1   e4   e7
    //           /     /  //   /
    //         /  /   e5  e8   e9
    //        e2 e3   /
    //               e6
    //
    // C := {e0} and `Explore(C, D, A)` picked `e4`. At this
    // point, UDPOR finished its recursive search of {e0, e4}
    // after having finished {e0, e1} prior.
    //
    // Thus the computation is (since D = {e1})
    //
    // Alt(C, D + {e}) --> Alt({e0}, {e1, e4})
    //
    // where U is given above. There is one alternative in this
    // case, viz {e0, e7, e9} since
    // 1. e9 conflicts with e4 in D
    // 2. e7 conflicts with e1 in D
    // 3. the set {e7, e9} is conflict-free since `e7 < e9`
    // 4. all other events are eliminated since their histories
    // intersect D
    //
    // UDPOR will continue its recursive search following `e7`
    // and add `e4` to D
    const Configuration C{e0_handle};
    const EventSet D_plus_e{e1_handle, e4_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e6));
    U.insert(std::move(e7));
    U.insert(std::move(e8));
    U.insert(std::move(e9));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE(alternative.has_value());
    REQUIRE(alternative.value().get_events() == EventSet({e0_handle, e7_handle, e9_handle}));
  }

  SECTION("Alternative computation call 9")
  {
    // During the ninth call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                  e0
    //              /  /   /
    //            e1   e4   e7
    //           /     /  //   /
    //         /  /   e5  e8   e9
    //        e2 e3   /        /
    //               e6       e10
    //
    // C := {e0, e7, e9} and `Explore(C, D, A)` picked `e10`.
    // (since en(C') where C' := {e0, e7, e9, e10} is empty
    // [so UDPOR will simply return when C' is reached]).
    //
    // Thus the computation is (since D = {e1, e4} [see the previous step])
    //
    // Alt(C, D + {e}) --> Alt({e0}, {e1, e4, e10})
    //
    // where U is given above. There are no alternatives in this case
    const Configuration C{e0_handle, e7_handle, e9_handle};
    const EventSet D_plus_e{e1_handle, e4_handle, e10_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e6));
    U.insert(std::move(e7));
    U.insert(std::move(e8));
    U.insert(std::move(e9));
    U.insert(std::move(e10));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE_FALSE(alternative.has_value());
  }

  SECTION("Alternative computation call 10")
  {
    // During the tenth call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                  e0
    //              /  /   /
    //            e1   e4   e7
    //           /     /  //   /
    //         /  /   e5  e8   e9
    //        e2 e3   /        /
    //               e6       e10
    //
    // C := {e0, e7} and `Explore(C, D, A)` picked `e9`.
    //
    // Thus the computation is (since D = {e1, e4} [see call 8])
    //
    // Alt(C, D + {e}) --> Alt({e0}, {e1, e4, e9})
    //
    // where U is given above. There are no alternatives in this case
    const Configuration C{e0_handle, e7_handle};
    const EventSet D_plus_e{e1_handle, e4_handle, e9_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e6));
    U.insert(std::move(e7));
    U.insert(std::move(e8));
    U.insert(std::move(e9));
    U.insert(std::move(e10));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE_FALSE(alternative.has_value());
  }

  SECTION("Alternative computation call 11 (final call)")
  {
    // During the eleventh and final call to Alt(C, D + {e}),
    // UDPOR believes `U` to be the following:
    //
    //                  e0
    //              /  /   /
    //            e1   e4   e7
    //           /     /  //   /
    //         /  /   e5  e8   e9
    //        e2 e3   /        /
    //               e6       e10
    //
    // C := {e0} and `Explore(C, D, A)` picked `e7`.
    //
    // Thus the computation is (since D = {e1, e4} [see call 8])
    //
    // Alt(C, D + {e}) --> Alt({e0}, {e1, e4, e7})
    //
    // where U is given above. There are no alternatives in this case:
    // everyone is eliminated!
    const Configuration C{e0_handle, e7_handle};
    const EventSet D_plus_e{e1_handle, e4_handle, e9_handle};

    REQUIRE(U.empty());
    U.insert(std::move(e0));
    U.insert(std::move(e1));
    U.insert(std::move(e2));
    U.insert(std::move(e3));
    U.insert(std::move(e4));
    U.insert(std::move(e6));
    U.insert(std::move(e7));
    U.insert(std::move(e8));
    U.insert(std::move(e9));
    U.insert(std::move(e10));

    const auto alternative = C.compute_alternative_to(D_plus_e, U);
    REQUIRE_FALSE(alternative.has_value());
  }

  SECTION("Alternative computation next")
  {
    SECTION("Followed {e0, e7} first")
    {
      const EventSet D{e1_handle, e7_handle};
      const Configuration C{e0_handle};

      REQUIRE(U.empty());
      U.insert(std::move(e0));
      U.insert(std::move(e1));
      U.insert(std::move(e2));
      U.insert(std::move(e3));
      U.insert(std::move(e4));
      U.insert(std::move(e5));
      U.insert(std::move(e7));
      U.insert(std::move(e8));
      U.insert(std::move(e9));
      U.insert(std::move(e10));

      const auto alternative = C.compute_alternative_to(D, U);
      REQUIRE(alternative.has_value());

      // In this case, only {e0, e4} is a valid alternative
      REQUIRE(alternative.value().get_events() == EventSet({e0_handle, e4_handle, e5_handle}));
    }

    SECTION("Followed {e0, e4} first")
    {
      const EventSet D{e1_handle, e4_handle};
      const Configuration C{e0_handle};

      REQUIRE(U.empty());
      U.insert(std::move(e0));
      U.insert(std::move(e1));
      U.insert(std::move(e2));
      U.insert(std::move(e3));
      U.insert(std::move(e4));
      U.insert(std::move(e5));
      U.insert(std::move(e6));
      U.insert(std::move(e7));
      U.insert(std::move(e8));
      U.insert(std::move(e9));

      const auto alternative = C.compute_alternative_to(D, U);
      REQUIRE(alternative.has_value());

      // In this case, only {e0, e7} is a valid alternative
      REQUIRE(alternative.value().get_events() == EventSet({e0_handle, e7_handle, e9_handle}));
    }
  }
}
