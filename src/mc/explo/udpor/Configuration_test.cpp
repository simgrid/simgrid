/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/maximal_subsets_iterator.hpp"

#include <unordered_map>

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
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e3{&e2};
  UnfoldingEvent e4{&e3};
  UnfoldingEvent e5{&e3};

  SECTION("Creating a configuration without events")
  {
    Configuration C;
    REQUIRE(C.get_events().empty());
    REQUIRE(C.get_latest_event() == nullptr);
  }

  SECTION("Creating a configuration with events")
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
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e3{&e2};
  UnfoldingEvent e4{&e2};

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
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e3{&e2};
  UnfoldingEvent e4{&e3};

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
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e3{&e2};
  UnfoldingEvent e4{&e3};
  UnfoldingEvent e5{&e4};
  UnfoldingEvent e6{&e3};

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
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e8{&e1};
  UnfoldingEvent e3{&e2};
  UnfoldingEvent e4{&e3};
  UnfoldingEvent e5{&e4};
  UnfoldingEvent e6{&e4};
  UnfoldingEvent e7{&e2, &e8};
  UnfoldingEvent e9{&e6, &e7};
  UnfoldingEvent e10{&e7};
  UnfoldingEvent e11{&e8};
  UnfoldingEvent e12{&e5, &e9, &e10};
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
      for (auto* e_hist : history) {
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

      for (auto* e_hist : history) {
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
      auto ordered_events              = C.get_topologically_sorted_events();
      const EventSet ordered_event_set = EventSet(std::move(ordered_events));
      REQUIRE(events_seen == ordered_event_set);
    }

    SECTION("Reverse direction")
    {
      auto ordered_events              = C.get_topologically_sorted_events_of_reverse_graph();
      const EventSet ordered_event_set = EventSet(std::move(ordered_events));
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
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e3{&e2};
  UnfoldingEvent e4{&e3};
  UnfoldingEvent e5{&e1};
  UnfoldingEvent e6{&e5};
  UnfoldingEvent e7{&e6};
  UnfoldingEvent e8{&e6};

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

      maximal_subsets_iterator first(C, [&](const UnfoldingEvent* e) { return interesting_bunch.contains(e); });
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

      maximal_subsets_iterator first(C, [&](const UnfoldingEvent* e) { return interesting_bunch.contains(e); });
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
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e3{&e1};
  UnfoldingEvent e4{&e2};
  UnfoldingEvent e5{&e2};
  UnfoldingEvent e6{&e3};
  UnfoldingEvent e7{&e3};
  UnfoldingEvent e8{&e4};
  UnfoldingEvent e9{&e4, &e5, &e6};
  UnfoldingEvent e10{&e6, &e7};
  UnfoldingEvent e11{&e8};
  UnfoldingEvent e12{&e8};
  UnfoldingEvent e13{&e9};
  UnfoldingEvent e14{&e9};
  UnfoldingEvent e15{&e10};
  UnfoldingEvent e16{&e5, &e11};
  UnfoldingEvent e17{&e12, &e13, &e14};
  UnfoldingEvent e18{&e14, &e15};
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