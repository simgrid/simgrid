/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "activity-lifecycle.hpp"

TEST_CASE("Activity lifecycle: sleep activities")
{
  XBT_INFO("#####[ launch next \"sleep\" test ]#####");

  BEGIN_SECTION("sleep")
  {
    XBT_INFO("Launch a sleep(5), and let it proceed");
    bool global = false;

    simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5", all_hosts[1], [&global]() {
      assert_exit(true, 5);
      simgrid::s4u::this_actor::sleep_for(5);
      global = true;
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Did the forked actor modify the global after sleeping, or was it killed before?");
    REQUIRE(global);

    END_SECTION;
  }

  BEGIN_SECTION("sleep killed at start")
  {
    XBT_INFO("Launch a sleep(5), and kill it right after start");
    simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5_killed", all_hosts[1], []() {
      assert_exit(false, 0);
      simgrid::s4u::this_actor::sleep_for(5);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    sleeper5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("sleep killed in middle")
  {
    XBT_INFO("Launch a sleep(5), and kill it after 2 secs");
    simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5_killed", all_hosts[1], []() {
      assert_exit(false, 2);
      simgrid::s4u::this_actor::sleep_for(5);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    sleeper5->kill();

    END_SECTION;
  }

  /* We cannot kill right at the end of the action because killer actors are always rescheduled to the end of the round
   * to avoid that they exit before their victim dereferences their name */

  BEGIN_SECTION("sleep restarted at start")
  {
    XBT_INFO("Launch a sleep(5), and restart its host right after start");
    simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5_restarted", all_hosts[1], []() {
      assert_exit(false, 0);
      simgrid::s4u::this_actor::sleep_for(5);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    sleeper5->get_host()->turn_off();
    sleeper5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sleep restarted in middle")
  {
    XBT_INFO("Launch a sleep(5), and restart its host after 2 secs");
    simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5_restarted", all_hosts[1], []() {
      assert_exit(false, 2);
      simgrid::s4u::this_actor::sleep_for(5);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    sleeper5->get_host()->turn_off();
    sleeper5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sleep restarted at end")
  {
    XBT_INFO("Launch a sleep(5), and restart its host right when it stops");
    bool sleeper_done = false;

    simgrid::s4u::Actor::create("sleep5_restarted", all_hosts[1], [&sleeper_done]() {
      assert_exit(true, 5);
      simgrid::s4u::this_actor::sleep_for(5);
      sleeper_done = true;
    });

    simgrid::s4u::Actor::create("killer", all_hosts[0], []() {
      simgrid::s4u::this_actor::sleep_for(5);
      XBT_VERB("Killer!");
      all_hosts[1]->turn_off();
      all_hosts[1]->turn_on();
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Was restarted actor already dead in the scheduling round during which the host_off simcall was issued?");
    REQUIRE(sleeper_done);

    END_SECTION;
  }

  BEGIN_SECTION("turn off its own host")
  {
    XBT_INFO("Launch a sleep(5), then saw off the branch it's sitting on");
    simgrid::s4u::Actor::create("sleep5_restarted", all_hosts[1], []() {
      assert_exit(false, 5);
      simgrid::s4u::this_actor::sleep_for(5);
      simgrid::s4u::this_actor::get_host()->turn_off();
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(9);
    all_hosts[1]->turn_on();

    END_SECTION;
  }

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}
