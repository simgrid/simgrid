/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"

using namespace simgrid::mc::udpor;

TEST_CASE("simgrid::mc::udpor::History: History generation")
{
  // The following tests concern the given event tree
  //        e1
  //    /      /
  //  e2        e6
  //  | \  \  /   /
  // e3 e4 e5      e7
  UnfoldingEvent e1;
  UnfoldingEvent e2{&e1};
  UnfoldingEvent e6{&e1};
  UnfoldingEvent e3{&e2};
  UnfoldingEvent e4{&e2};
  UnfoldingEvent e5{&e2, &e6};
  UnfoldingEvent e7{&e6};

  SECTION("History with no events")
  {
    History history;
    REQUIRE(history.get_all_events() == EventSet());
    REQUIRE_FALSE(history.contains(&e1));
    REQUIRE_FALSE(history.contains(&e2));
    REQUIRE_FALSE(history.contains(&e3));
    REQUIRE_FALSE(history.contains(&e4));
    REQUIRE_FALSE(history.contains(&e5));
    REQUIRE_FALSE(history.contains(&e6));
    REQUIRE_FALSE(history.contains(&e7));
  }

  SECTION("Histories with a single event")
  {
    SECTION("Root event's history has a single event")
    {
      History history(&e1);
      REQUIRE(history.get_all_events() == EventSet({&e1}));
    }

    SECTION("Check node e2")
    {
      History history(&e2);
      REQUIRE(history.get_all_events() == EventSet({&e1, &e2}));
      REQUIRE(history.contains(&e1));
      REQUIRE(history.contains(&e2));
      REQUIRE_FALSE(history.contains(&e3));
      REQUIRE_FALSE(history.contains(&e4));
      REQUIRE_FALSE(history.contains(&e5));
      REQUIRE_FALSE(history.contains(&e6));
      REQUIRE_FALSE(history.contains(&e7));
    }

    SECTION("Check node e3")
    {
      History history(&e3);
      REQUIRE(history.get_all_events() == EventSet({&e1, &e2, &e3}));
      REQUIRE(history.contains(&e1));
      REQUIRE(history.contains(&e2));
      REQUIRE(history.contains(&e3));
      REQUIRE_FALSE(history.contains(&e4));
      REQUIRE_FALSE(history.contains(&e5));
      REQUIRE_FALSE(history.contains(&e6));
      REQUIRE_FALSE(history.contains(&e7));
    }

    SECTION("Check node e4")
    {
      History history(&e4);
      REQUIRE(history.get_all_events() == EventSet({&e1, &e2, &e4}));
      REQUIRE(history.contains(&e1));
      REQUIRE(history.contains(&e2));
      REQUIRE_FALSE(history.contains(&e3));
      REQUIRE(history.contains(&e4));
      REQUIRE_FALSE(history.contains(&e5));
      REQUIRE_FALSE(history.contains(&e6));
      REQUIRE_FALSE(history.contains(&e7));
    }

    SECTION("Check node e5")
    {
      History history(&e5);
      REQUIRE(history.get_all_events() == EventSet({&e1, &e2, &e6, &e5}));
      REQUIRE(history.contains(&e1));
      REQUIRE(history.contains(&e2));
      REQUIRE_FALSE(history.contains(&e3));
      REQUIRE_FALSE(history.contains(&e4));
      REQUIRE(history.contains(&e5));
      REQUIRE(history.contains(&e6));
      REQUIRE_FALSE(history.contains(&e7));
    }

    SECTION("Check node e6")
    {
      History history(&e6);
      REQUIRE(history.get_all_events() == EventSet({&e1, &e6}));
      REQUIRE(history.contains(&e1));
      REQUIRE_FALSE(history.contains(&e2));
      REQUIRE_FALSE(history.contains(&e3));
      REQUIRE_FALSE(history.contains(&e4));
      REQUIRE_FALSE(history.contains(&e5));
      REQUIRE(history.contains(&e6));
      REQUIRE_FALSE(history.contains(&e7));
    }

    SECTION("Check node e7")
    {
      History history(&e7);
      REQUIRE(history.get_all_events() == EventSet({&e1, &e6, &e7}));
      REQUIRE(history.contains(&e1));
      REQUIRE_FALSE(history.contains(&e2));
      REQUIRE_FALSE(history.contains(&e3));
      REQUIRE_FALSE(history.contains(&e4));
      REQUIRE_FALSE(history.contains(&e5));
      REQUIRE(history.contains(&e6));
      REQUIRE(history.contains(&e7));
    }
  }

  SECTION("Histories with multiple nodes")
  {
    SECTION("Nodes contained in the same branch")
    {
      History history_e1e2(EventSet({&e1, &e2}));
      REQUIRE(history_e1e2.get_all_events() == EventSet({&e1, &e2}));
      REQUIRE(history_e1e2.contains(&e1));
      REQUIRE(history_e1e2.contains(&e2));
      REQUIRE_FALSE(history_e1e2.contains(&e3));
      REQUIRE_FALSE(history_e1e2.contains(&e4));
      REQUIRE_FALSE(history_e1e2.contains(&e5));
      REQUIRE_FALSE(history_e1e2.contains(&e6));
      REQUIRE_FALSE(history_e1e2.contains(&e7));

      History history_e1e3(EventSet({&e1, &e3}));
      REQUIRE(history_e1e3.get_all_events() == EventSet({&e1, &e2, &e3}));
      REQUIRE(history_e1e3.contains(&e1));
      REQUIRE(history_e1e3.contains(&e2));
      REQUIRE(history_e1e3.contains(&e3));
      REQUIRE_FALSE(history_e1e3.contains(&e4));
      REQUIRE_FALSE(history_e1e3.contains(&e5));
      REQUIRE_FALSE(history_e1e3.contains(&e6));
      REQUIRE_FALSE(history_e1e3.contains(&e7));

      History history_e6e7(EventSet({&e6, &e7}));
      REQUIRE(history_e6e7.get_all_events() == EventSet({&e1, &e6, &e7}));
      REQUIRE(history_e6e7.contains(&e1));
      REQUIRE_FALSE(history_e6e7.contains(&e2));
      REQUIRE_FALSE(history_e6e7.contains(&e3));
      REQUIRE_FALSE(history_e6e7.contains(&e4));
      REQUIRE_FALSE(history_e6e7.contains(&e5));
      REQUIRE(history_e6e7.contains(&e6));
      REQUIRE(history_e6e7.contains(&e7));
    }

    SECTION("Nodes with the same ancestor")
    {
      History history_e3e5(EventSet({&e3, &e5}));
      REQUIRE(history_e3e5.get_all_events() == EventSet({&e1, &e2, &e3, &e5, &e6}));
      REQUIRE(history_e3e5.contains(&e1));
      REQUIRE(history_e3e5.contains(&e2));
      REQUIRE(history_e3e5.contains(&e3));
      REQUIRE_FALSE(history_e3e5.contains(&e4));
      REQUIRE(history_e3e5.contains(&e5));
      REQUIRE(history_e3e5.contains(&e6));
      REQUIRE_FALSE(history_e3e5.contains(&e7));
    }

    SECTION("Nodes with different ancestors")
    {
      History history_e4e7(EventSet({&e4, &e7}));
      REQUIRE(history_e4e7.get_all_events() == EventSet({&e1, &e2, &e4, &e6, &e7}));
      REQUIRE(history_e4e7.contains(&e1));
      REQUIRE(history_e4e7.contains(&e2));
      REQUIRE_FALSE(history_e4e7.contains(&e3));
      REQUIRE(history_e4e7.contains(&e4));
      REQUIRE_FALSE(history_e4e7.contains(&e5));
      REQUIRE(history_e4e7.contains(&e6));
      REQUIRE(history_e4e7.contains(&e7));
    }

    SECTION("Large number of nodes")
    {
      History history_e2356(EventSet({&e2, &e3, &e5, &e6}));
      REQUIRE(history_e2356.get_all_events() == EventSet({&e1, &e2, &e3, &e5, &e6}));
      REQUIRE(history_e2356.contains(&e1));
      REQUIRE(history_e2356.contains(&e2));
      REQUIRE(history_e2356.contains(&e3));
      REQUIRE_FALSE(history_e2356.contains(&e4));
      REQUIRE(history_e2356.contains(&e5));
      REQUIRE(history_e2356.contains(&e6));
      REQUIRE_FALSE(history_e2356.contains(&e7));
    }

    SECTION("History of the entire graph yields the entire graph")
    {
      History history(EventSet({&e1, &e2, &e3, &e4, &e5, &e6, &e7}));
      REQUIRE(history.get_all_events() == EventSet({&e1, &e2, &e3, &e4, &e5, &e6, &e7}));
      REQUIRE(history.contains(&e1));
      REQUIRE(history.contains(&e2));
      REQUIRE(history.contains(&e3));
      REQUIRE(history.contains(&e4));
      REQUIRE(history.contains(&e5));
      REQUIRE(history.contains(&e6));
      REQUIRE(history.contains(&e7));
    }
  }
}
