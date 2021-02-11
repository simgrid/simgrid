/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "activity-lifecycle.hpp"

// Normally, we should be able use Catch2's REQUIRE_THROWS_AS(...), but it generates errors with Address Sanitizer.
// They're certainly false positive. Nevermind and use this simpler replacement.
#define REQUIRE_NETWORK_FAILURE(...)                                                                                   \
  do {                                                                                                                 \
    try {                                                                                                              \
      __VA_ARGS__;                                                                                                     \
      FAIL("Expected exception NetworkFailureException not caught");                                                   \
    } catch (simgrid::NetworkFailureException const&) {                                                                \
      XBT_VERB("got expected NetworkFailureException");                                                                \
    }                                                                                                                  \
  } while (0)

TEST_CASE("Activity lifecycle: direct communication activities")
{
  XBT_INFO("#####[ launch next \"direct-comm\" test ]#####");

  BEGIN_SECTION("dcomm")
  {
    XBT_INFO("Launch a dcomm(5s), and let it proceed");
    bool global = false;

    simgrid::s4u::ActorPtr dcomm5 = simgrid::s4u::Actor::create("dcomm5", all_hosts[1], [&global]() {
      assert_exit(true, 5.);
      all_hosts[1]->sendto(all_hosts[2], 5000);
      global = true;
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Did the forked actor modify the global after sleeping, or was it killed before?");
    REQUIRE(global);

    END_SECTION;
  }

  BEGIN_SECTION("dcomm actor killed at start")
  {
    XBT_INFO("Launch a dcomm(5s), and kill it right after start");
    simgrid::s4u::ActorPtr dcomm5 = simgrid::s4u::Actor::create("dcomm5_killed", all_hosts[1], []() {
      assert_exit(false, 0);
      all_hosts[1]->sendto(all_hosts[2], 5000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    dcomm5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("dcomm actor killed in middle")
  {
    XBT_INFO("Launch a dcomm(5s), and kill it after 2 secs");
    simgrid::s4u::ActorPtr dcomm5 = simgrid::s4u::Actor::create("dcomm5_killed", all_hosts[1], []() {
      assert_exit(false, 2);
      all_hosts[1]->sendto(all_hosts[2], 5000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    dcomm5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("dcomm host restarted at start")
  {
    XBT_INFO("Launch a dcomm(5s), and restart its host right after start");
    simgrid::s4u::ActorPtr dcomm5 = simgrid::s4u::Actor::create("dcomm5_restarted", all_hosts[1], []() {
      assert_exit(false, 0);
      all_hosts[1]->sendto(all_hosts[2], 5000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    dcomm5->get_host()->turn_off();
    dcomm5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("dcomm host restarted in middle")
  {
    XBT_INFO("Launch a dcomm(5s), and restart its host after 2 secs");
    simgrid::s4u::ActorPtr dcomm5 = simgrid::s4u::Actor::create("dcomm5_restarted", all_hosts[1], []() {
      assert_exit(false, 2);
      all_hosts[1]->sendto(all_hosts[2], 5000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    dcomm5->get_host()->turn_off();
    dcomm5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("dcomm host restarted at end")
  {
    XBT_INFO("Launch a dcomm(5s), and restart its host right when it stops");
    bool execution_done = false;

    simgrid::s4u::Actor::create("dcomm5_restarted", all_hosts[1], [&execution_done]() {
      assert_exit(true, 5);
      all_hosts[1]->sendto(all_hosts[2], 5000);
      execution_done = true;
    });

    simgrid::s4u::Actor::create("killer", all_hosts[0], []() {
      simgrid::s4u::this_actor::sleep_for(5);
      XBT_VERB("Killer!");
      all_hosts[1]->turn_off();
      all_hosts[1]->turn_on();
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Was restarted actor already dead in the scheduling round during which the host_off simcall was issued?");
    REQUIRE(execution_done);

    END_SECTION;
  }

  BEGIN_SECTION("dcomm link restarted at start")
  {
    XBT_INFO("Launch a dcomm(5s), and restart the used link right after start");
    simgrid::s4u::ActorPtr dcomm5 = simgrid::s4u::Actor::create("dcomm5_restarted", all_hosts[1], []() {
      assert_exit(true, 0);
      REQUIRE_NETWORK_FAILURE(all_hosts[1]->sendto(all_hosts[2], 5000));
    });

    simgrid::s4u::this_actor::yield();
    auto link = simgrid::s4u::Engine::get_instance()->link_by_name("link1");
    link->turn_off();
    link->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("dcomm link restarted in middle")
  {
    XBT_INFO("Launch a dcomm(5s), and restart the used link after 2 secs");
    simgrid::s4u::ActorPtr dcomm5 = simgrid::s4u::Actor::create("dcomm5_restarted", all_hosts[1], []() {
      assert_exit(true, 2);
      REQUIRE_NETWORK_FAILURE(all_hosts[1]->sendto(all_hosts[2], 5000));
    });

    simgrid::s4u::this_actor::sleep_for(2);
    auto link = simgrid::s4u::Engine::get_instance()->link_by_name("link1");
    link->turn_off();
    link->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("dcomm link restarted at end")
  {
    XBT_INFO("Launch a dcomm(5s), and restart the used link right when it stops");
    bool execution_done = false;

    simgrid::s4u::Actor::create("dcomm5_restarted", all_hosts[1], [&execution_done]() {
      assert_exit(true, 5);
      all_hosts[1]->sendto(all_hosts[2], 5000);
      execution_done = true;
    });

    simgrid::s4u::Actor::create("killer", all_hosts[0], []() {
      simgrid::s4u::this_actor::sleep_for(5);
      XBT_VERB("Killer!");
      auto link = simgrid::s4u::Engine::get_instance()->link_by_name("link1");
      link->turn_off();
      link->turn_on();
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Was restarted actor already dead in the scheduling round during which the host_off simcall was issued?");
    REQUIRE(execution_done);

    END_SECTION;
  }

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}
