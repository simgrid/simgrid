/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch_simgrid.hpp"

TEST_CASE("Activity lifecycle: direct communication (sendto) activities")
{
  XBT_INFO("#####[ launch next \"direct-comm\" test ]#####");

  BEGIN_SECTION("sendto")
  {
    XBT_INFO("Launch a sendto(5s), and let it proceed");
    bool global = false;

    simgrid::s4u::ActorPtr sendto5 = simgrid::s4u::Actor::create("sendto5", all_hosts[1], [&global]() {
      assert_exit(true, 5.);
      simgrid::s4u::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
      global = true;
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Did the forked actor modify the global after sleeping, or was it killed before?");
    REQUIRE(global);

    END_SECTION;
  }

  BEGIN_SECTION("sendto actor killed at start")
  {
    XBT_INFO("Launch a sendto(5s), and kill it right after start");
    simgrid::s4u::ActorPtr sendto5 = simgrid::s4u::Actor::create("sendto5_killed", all_hosts[1], []() {
      assert_exit(false, 0);
      simgrid::s4u::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    sendto5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("sendto actor killed in middle")
  {
    XBT_INFO("Launch a sendto(5s), and kill it after 2 secs");
    simgrid::s4u::ActorPtr sendto5 = simgrid::s4u::Actor::create("sendto5_killed", all_hosts[1], []() {
      assert_exit(false, 2);
      simgrid::s4u::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    sendto5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("sendto host restarted at start")
  {
    XBT_INFO("Launch a sendto(5s), and restart its host right after start");
    simgrid::s4u::ActorPtr sendto5 = simgrid::s4u::Actor::create("sendto5_restarted", all_hosts[1], []() {
      assert_exit(false, 0);
      simgrid::s4u::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    sendto5->get_host()->turn_off();
    sendto5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sendto host restarted in middle")
  {
    XBT_INFO("Launch a sendto(5s), and restart its host after 2 secs");
    simgrid::s4u::ActorPtr sendto5 = simgrid::s4u::Actor::create("sendto5_restarted", all_hosts[1], []() {
      assert_exit(false, 2);
      simgrid::s4u::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    sendto5->get_host()->turn_off();
    sendto5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sendto host restarted at end")
  {
    XBT_INFO("Launch a sendto(5s), and restart its host right when it stops");
    bool execution_done = false;

    simgrid::s4u::Actor::create("sendto5_restarted", all_hosts[1], [&execution_done]() {
      assert_exit(true, 5);
      simgrid::s4u::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
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

  BEGIN_SECTION("sendto link restarted at start")
  {
    XBT_INFO("Launch a sendto(5s), and restart the used link right after start");
    simgrid::s4u::ActorPtr sendto5 = simgrid::s4u::Actor::create("sendto5_restarted", all_hosts[1], []() {
      assert_exit(true, 0);
      REQUIRE_NETWORK_FAILURE(simgrid::s4u::Comm::sendto(all_hosts[1], all_hosts[2], 5000));
    });

    simgrid::s4u::this_actor::yield();
    auto link = simgrid::s4u::Engine::get_instance()->link_by_name("link1");
    link->turn_off();
    link->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sendto link restarted in middle")
  {
    XBT_INFO("Launch a sendto(5s), and restart the used link after 2 secs");
    simgrid::s4u::ActorPtr sendto5 = simgrid::s4u::Actor::create("sendto5_restarted", all_hosts[1], []() {
      assert_exit(true, 2);
      REQUIRE_NETWORK_FAILURE(simgrid::s4u::Comm::sendto(all_hosts[1], all_hosts[2], 5000));
    });

    simgrid::s4u::this_actor::sleep_for(2);
    auto link = simgrid::s4u::Engine::get_instance()->link_by_name("link1");
    link->turn_off();
    link->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sendto link restarted at end")
  {
    XBT_INFO("Launch a sendto(5s), and restart the used link right when it stops");
    bool execution_done = false;

    simgrid::s4u::Actor::create("sendto5_restarted", all_hosts[1], [&execution_done]() {
      assert_exit(true, 5);
      simgrid::s4u::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
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
