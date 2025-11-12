/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "teshsuite/catch_simgrid.hpp"
namespace sg4=simgrid::s4u;

TEST_CASE("Activity lifecycle: direct communication (sendto) activities")
{
  XBT_INFO("#####[ launch next \"direct-comm\" test ]#####");

  BEGIN_SECTION("sendto")
  {
    XBT_INFO("Launch a sendto(5s), and let it proceed");
    bool global = false;

    sg4::ActorPtr sendto5 =
        all_hosts[1]->add_actor("sendto5", [&global]() {
          assert_exit(true, 5.);
          sg4::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
          global = true;
        });

    sg4::this_actor::sleep_for(9);
    INFO("Did the forked actor modify the global after sleeping, or was it killed before?");
    REQUIRE(global);

    END_SECTION;
  }

  BEGIN_SECTION("sendto actor killed at start")
  {
    XBT_INFO("Launch a sendto(5s), and kill it right after start");
    sg4::ActorPtr sendto5 =
        all_hosts[1]->add_actor("sendto5_killed", []() {
          assert_exit(false, 0);
          sg4::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
          FAIL("I should be dead now");
        });

    sg4::this_actor::yield();
    sendto5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("sendto actor killed in middle")
  {
    XBT_INFO("Launch a sendto(5s), and kill it after 2 secs");
    sg4::ActorPtr sendto5 =
        all_hosts[1]->add_actor("sendto5_killed", []() {
          assert_exit(false, 2);
          sg4::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
          FAIL("I should be dead now");
        });

    sg4::this_actor::sleep_for(2);
    sendto5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("sendto host restarted at start")
  {
    XBT_INFO("Launch a sendto(5s), and restart its host right after start");
    sg4::ActorPtr sendto5 =
        all_hosts[1]->add_actor("sendto5_restarted", []() {
          assert_exit(false, 0);
          sg4::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
          FAIL("I should be dead now");
        });

    sg4::this_actor::yield();
    sendto5->get_host()->turn_off();
    sendto5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sendto host restarted in middle")
  {
    XBT_INFO("Launch a sendto(5s), and restart its host after 2 secs");
    sg4::ActorPtr sendto5 =
        all_hosts[1]->add_actor("sendto5_restarted", []() {
          assert_exit(false, 2);
          sg4::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
          FAIL("I should be dead now");
        });

    sg4::this_actor::sleep_for(2);
    sendto5->get_host()->turn_off();
    sendto5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sendto host restarted at end")
  {
    XBT_INFO("Launch a sendto(5s), and restart its host right when it stops");
    bool execution_done = false;

    all_hosts[1]->add_actor("sendto5_restarted", [&execution_done]() {
      assert_exit(true, 5);
      sg4::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
      execution_done = true;
    });

    all_hosts[0]->add_actor("killer", []() {
      sg4::this_actor::sleep_for(5);
      XBT_VERB("Killer!");
      all_hosts[1]->turn_off();
      all_hosts[1]->turn_on();
    });

    sg4::this_actor::sleep_for(9);
    INFO("Was restarted actor already dead in the scheduling round during which the host_off simcall was issued?");
    REQUIRE(execution_done);

    END_SECTION;
  }

  BEGIN_SECTION("sendto link restarted at start")
  {
    XBT_INFO("Launch a sendto(5s), and restart the used link right after start");
    sg4::ActorPtr sendto5 =
        all_hosts[1]->add_actor("sendto5_restarted", []() {
          assert_exit(true, 0);
          REQUIRE_NETWORK_FAILURE(sg4::Comm::sendto(all_hosts[1], all_hosts[2], 5000));
        });

    sg4::this_actor::yield();
    auto* link = sg4::Engine::get_instance()->link_by_name("link1");
    link->turn_off();
    link->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sendto link restarted in middle")
  {
    XBT_INFO("Launch a sendto(5s), and restart the used link after 2 secs");
    sg4::ActorPtr sendto5 =
        all_hosts[1]->add_actor("sendto5_restarted", []() {
          assert_exit(true, 2);
          REQUIRE_NETWORK_FAILURE(sg4::Comm::sendto(all_hosts[1], all_hosts[2], 5000));
        });

    sg4::this_actor::sleep_for(2);
    auto* link = sg4::Engine::get_instance()->link_by_name("link1");
    link->turn_off();
    link->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sendto link restarted at end")
  {
    XBT_INFO("Launch a sendto(5s), and restart the used link right when it stops");
    bool execution_done = false;

    all_hosts[1]->add_actor("sendto5_restarted", [&execution_done]() {
      assert_exit(true, 5);
      sg4::Comm::sendto(all_hosts[1], all_hosts[2], 5000);
      execution_done = true;
    });

    all_hosts[0]->add_actor("killer", []() {
      sg4::this_actor::sleep_for(5);
      XBT_VERB("Killer!");
      auto* link = sg4::Engine::get_instance()->link_by_name("link1");
      link->turn_off();
      link->turn_on();
    });

    sg4::this_actor::sleep_for(9);
    INFO("Was restarted actor already dead in the scheduling round during which the host_off simcall was issued?");
    REQUIRE(execution_done);

    END_SECTION;
  }

  sg4::this_actor::sleep_for(10);
  assert_cleanup();
}
