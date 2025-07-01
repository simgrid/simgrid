/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "teshsuite/catch_simgrid.hpp"
namespace sg4 = simgrid::s4u;

TEST_CASE("Activity lifecycle: exec activities")
{
  XBT_INFO("#####[ launch next \"exec\" test ]#####");

  BEGIN_SECTION("exec")
  {
    XBT_INFO("Launch an execute(5s), and let it proceed");
    bool global = false;

    sg4::ActorPtr exec5 = all_hosts[1]->add_actor("exec5", [&global]() {
      assert_exit(true, 5.);
      sg4::this_actor::execute(500000000);
      global = true;
    });

    sg4::this_actor::sleep_for(9);
    INFO("Did the forked actor modify the global after sleeping, or was it killed before?");
    REQUIRE(global);

    END_SECTION;
  }

  BEGIN_SECTION("exec killed at start")
  {
    XBT_INFO("Launch an execute(5s), and kill it right after start");
    sg4::ActorPtr exec5 = all_hosts[1]->add_actor("exec5_killed", []() {
      assert_exit(false, 0);
      sg4::this_actor::execute(500000000);
      FAIL("I should be dead now");
    });

    sg4::this_actor::yield();
    exec5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("exec killed in middle")
  {
    XBT_INFO("Launch an execute(5s), and kill it after 2 secs");
    sg4::ActorPtr exec5 = all_hosts[1]->add_actor("exec5_killed", []() {
      assert_exit(false, 2);
      sg4::this_actor::execute(500000000);
      FAIL("I should be dead now");
    });

    sg4::this_actor::sleep_for(2);
    exec5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("exec restarted at start")
  {
    XBT_INFO("Launch an execute(5s), and restart its host right after start");
    sg4::ActorPtr exec5 = all_hosts[1]->add_actor("exec5_restarted", []() {
          assert_exit(false, 0);
          sg4::this_actor::execute(500000000);
          FAIL("I should be dead now");
        });

    sg4::this_actor::yield();
    exec5->get_host()->turn_off();
    exec5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("exec restarted in middle")
  {
    XBT_INFO("Launch an execute(5s), and restart its host after 2 secs");
    sg4::ActorPtr exec5 = all_hosts[1]->add_actor("exec5_restarted", []() {
          assert_exit(false, 2);
          sg4::this_actor::execute(500000000);
          FAIL("I should be dead now");
        });

    sg4::this_actor::sleep_for(2);
    exec5->get_host()->turn_off();
    exec5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("exec restarted at end")
  {
    XBT_INFO("Launch an execute(5s), and restart its host right when it stops");
    bool execution_done = false;

    all_hosts[1]->add_actor("exec5_restarted", [&execution_done]() {
      assert_exit(true, 5);
      sg4::this_actor::execute(500000000);
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

  sg4::this_actor::sleep_for(10);
  assert_cleanup();
}
