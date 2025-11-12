/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "teshsuite/catch_simgrid.hpp"
namespace sg4 = simgrid::s4u;

TEST_CASE("Activity lifecycle: sleep activities")
{
  XBT_INFO("#####[ launch next \"sleep\" test ]#####");

  BEGIN_SECTION("sleep")
  {
    XBT_INFO("Launch a sleep(5), and let it proceed");
    bool global = false;

    sg4::ActorPtr sleeper5 = all_hosts[1]->add_actor("sleep5", [&global]() {
          assert_exit(true, 5);
          sg4::this_actor::sleep_for(5);
          global = true;
        });

    sg4::this_actor::sleep_for(9);
    INFO("Did the forked actor modify the global after sleeping, or was it killed before?");
    REQUIRE(global);

    END_SECTION;
  }

  BEGIN_SECTION("sleep killed at start")
  {
    XBT_INFO("Launch a sleep(5), and kill it right after start");
    sg4::ActorPtr sleeper5 = all_hosts[1]->add_actor("sleep5_killed", []() {
          assert_exit(false, 0);
          sg4::this_actor::sleep_for(5);
          FAIL("I should be dead now");
        });

    sg4::this_actor::yield();
    sleeper5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("sleep killed in middle")
  {
    XBT_INFO("Launch a sleep(5), and kill it after 2 secs");
    sg4::ActorPtr sleeper5 = all_hosts[1]->add_actor("sleep5_killed", []() {
          assert_exit(false, 2);
          sg4::this_actor::sleep_for(5);
          FAIL("I should be dead now");
        });

    sg4::this_actor::sleep_for(2);
    sleeper5->kill();

    END_SECTION;
  }

  /* We cannot kill right at the end of the action because killer actors are always rescheduled to the end of the round
   * to avoid that they exit before their victim dereferences their name */

  BEGIN_SECTION("sleep restarted at start")
  {
    XBT_INFO("Launch a sleep(5), and restart its host right after start");
    sg4::ActorPtr sleeper5 = all_hosts[1]->add_actor("sleep5_restarted", []() {
          assert_exit(false, 0);
          sg4::this_actor::sleep_for(5);
          FAIL("I should be dead now");
        });

    sg4::this_actor::yield();
    sleeper5->get_host()->turn_off();
    sleeper5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sleep restarted in middle")
  {
    XBT_INFO("Launch a sleep(5), and restart its host after 2 secs");
    sg4::ActorPtr sleeper5 = all_hosts[1]->add_actor("sleep5_restarted", []() {
          assert_exit(false, 2);
          sg4::this_actor::sleep_for(5);
          FAIL("I should be dead now");
        });

    sg4::this_actor::sleep_for(2);
    sleeper5->get_host()->turn_off();
    sleeper5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("sleep restarted at end")
  {
    XBT_INFO("Launch a sleep(5), and restart its host right when it stops");
    bool sleeper_done = false;

    all_hosts[1]->add_actor("sleep5_restarted", [&sleeper_done]() {
      assert_exit(true, 5);
      sg4::this_actor::sleep_for(5);
      sleeper_done = true;
    });

    all_hosts[0]->add_actor("killer", []() {
      sg4::this_actor::sleep_for(5);
      XBT_VERB("Killer!");
      all_hosts[1]->turn_off();
      all_hosts[1]->turn_on();
    });

    sg4::this_actor::sleep_for(9);
    INFO("Was restarted actor already dead in the scheduling round during which the host_off simcall was issued?");
    REQUIRE(sleeper_done);

    END_SECTION;
  }

  BEGIN_SECTION("turn off its own host")
  {
    XBT_INFO("Launch a sleep(5), then saw off the branch it's sitting on");
    all_hosts[1]->add_actor("sleep5_restarted", []() {
      assert_exit(false, 5);
      sg4::this_actor::sleep_for(5);
      sg4::this_actor::get_host()->turn_off();
      FAIL("I should be dead now");
    });

    sg4::this_actor::sleep_for(9);
    all_hosts[1]->turn_on();

    END_SECTION;
  }

  sg4::this_actor::sleep_for(10);
  assert_cleanup();
}
