/* Copyright (c) 2010-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "teshsuite/catch_simgrid.hpp"
#include "simgrid/Exception.hpp"

TEST_CASE("Activity lifecycle: io activities")
{
  XBT_INFO("#####[ launch next \"io\" test ]#####");

  BEGIN_SECTION("io_read")
  {
    XBT_INFO("Launch an read(5s), and let it proceed");
    bool global = false;
    auto disk = all_hosts[0]->get_disks().front();
    simgrid::s4u::ActorPtr io_read5 = simgrid::s4u::Actor::create("io_read5", all_hosts[0], [&global, disk]() {
      assert_exit(true, 5.);
      disk->read(500000000);
      global = true;
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Did the forked actor modify the global after sleeping, or was it killed before?");
    REQUIRE(global);

    END_SECTION;
  }

  BEGIN_SECTION("io_write")
  {
    XBT_INFO("Launch an write(5s), and let it proceed");
    bool global = false;
    auto disk = all_hosts[0]->get_disks().front();
    simgrid::s4u::ActorPtr io_write5 = simgrid::s4u::Actor::create("io_write5", all_hosts[0], [&global, disk]() {
      assert_exit(true, 5.);
      disk->write(250000000);
      global = true;
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Did the forked actor modify the global after sleeping, or was it killed before?");
    REQUIRE(global);

    END_SECTION;
  }

  BEGIN_SECTION("read killed at start")
  {
    XBT_INFO("Launch an read(5s), and kill it right after start");
    simgrid::s4u::ActorPtr io_read5 = simgrid::s4u::Actor::create("io_read5_killed", all_hosts[0], []() {
      assert_exit(false, 0);
      all_hosts[0]->get_disks().front()->read(500000000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    io_read5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("write killed at start")
  {
    XBT_INFO("Launch an write(5s), and kill it right after start");
    simgrid::s4u::ActorPtr io_write5 = simgrid::s4u::Actor::create("io_write5_killed", all_hosts[0], []() {
      assert_exit(false, 0);
      all_hosts[0]->get_disks().front()->write(250000000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    io_write5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("read killed in middle")
  {
    XBT_INFO("Launch an read(5s), and kill it after 2 secs");
    simgrid::s4u::ActorPtr io_read5 = simgrid::s4u::Actor::create("io_read5_killed", all_hosts[0], []() {
      assert_exit(false, 2);
      all_hosts[0]->get_disks().front()->read(500000000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    io_read5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("write killed in middle")
  {
    XBT_INFO("Launch an write(5s), and kill it after 2 secs");
    simgrid::s4u::ActorPtr io_write5 = simgrid::s4u::Actor::create("io_write5_killed", all_hosts[0], []() {
      assert_exit(false, 2);
      all_hosts[0]->get_disks().front()->write(250000000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    io_write5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("read restarted at start")
  {
    XBT_INFO("Launch an read(5s), and restart its disk right after start");
    auto disk = all_hosts[0]->get_disks().front();
    simgrid::s4u::ActorPtr io_read5 = simgrid::s4u::Actor::create("io_read_restarted", all_hosts[0], [disk]() {
      assert_exit(false, 0);
      REQUIRE_STORAGE_FAILURE({disk->read(500000000);});
    });

    simgrid::s4u::this_actor::yield();
    disk->turn_off();
    // Do not restart the disk right away or the finish of the I/O activity is called too late and we don't see the bug
    simgrid::s4u::this_actor::sleep_for(1);
    disk->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("write restarted at start")
  {
    XBT_INFO("Launch an write(5s), and restart its disk right after start");
    auto disk = all_hosts[0]->get_disks().front();
    simgrid::s4u::ActorPtr io_write5 = simgrid::s4u::Actor::create("io_write_restarted", all_hosts[0], [disk]() {
      assert_exit(false, 0);
      REQUIRE_STORAGE_FAILURE({disk->write(250000000);});
    });

    simgrid::s4u::this_actor::yield();
    disk->turn_off();
    simgrid::s4u::this_actor::sleep_for(1);
    disk->turn_on();
    END_SECTION;
  }

  BEGIN_SECTION("read stopped in middle")
  {
    XBT_INFO("Launch an read(5s), and turn off its disk after 2 secs");
    auto disk = all_hosts[0]->get_disks().front();
    simgrid::s4u::ActorPtr io_read5 = simgrid::s4u::Actor::create("io_read_restarted", all_hosts[0], [disk]() {
      assert_exit(false, 2);
      REQUIRE_STORAGE_FAILURE({disk->read(500000000);});
    });

    simgrid::s4u::this_actor::sleep_for(2);
    disk->turn_off();
    simgrid::s4u::this_actor::sleep_for(1);
    disk->turn_on();
    END_SECTION;
  }

  BEGIN_SECTION("write stopped in middle")
  {
    XBT_INFO("Launch an write(5s), and turn off its disk after 2 secs");
    auto disk = all_hosts[0]->get_disks().front();
    simgrid::s4u::ActorPtr io_write5 = simgrid::s4u::Actor::create("io_write_restarted", all_hosts[0], [disk]() {
      assert_exit(false, 2);
      REQUIRE_STORAGE_FAILURE({disk->write(250000000);});
    });

    simgrid::s4u::this_actor::sleep_for(2);
    disk->turn_off();
    simgrid::s4u::this_actor::sleep_for(1);
    disk->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("read restarted at end")
  {
    XBT_INFO("Launch an read(5s), and restart its disk right when it stops");
    bool read_done = false;
    auto disk = all_hosts[0]->get_disks().front();

    simgrid::s4u::Actor::create("io_read5_restarted", all_hosts[0], [&read_done, disk]() {
      assert_exit(true, 5);
      disk->read(500000000);
      read_done = true;
    });

    simgrid::s4u::Actor::create("killer", all_hosts[0], [disk]() {
      simgrid::s4u::this_actor::sleep_for(5);
      XBT_VERB("Killer!");
      disk->turn_off();
      simgrid::s4u::this_actor::sleep_for(1);
      disk->turn_on();
    });

    simgrid::s4u::this_actor::sleep_for(8);
    INFO("Was restarted actor already dead in the scheduling round during which the turn_off simcall was issued?");
    REQUIRE(read_done);

    END_SECTION;
  }

  BEGIN_SECTION("write restarted at end")
  {
    XBT_INFO("Launch an write(5s), and restart its disk right when it stops");
    bool write_done = false;
    auto disk = all_hosts[0]->get_disks().front();

    simgrid::s4u::Actor::create("io_write5_restarted", all_hosts[0], [&write_done, disk]() {
      assert_exit(true, 5);
        disk->write(250000000);
      write_done = true;
    });

    simgrid::s4u::Actor::create("killer", all_hosts[0], [disk]() {
      simgrid::s4u::this_actor::sleep_for(5);
      XBT_VERB("Killer!");
      disk->turn_off();
      disk->turn_on();
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Was restarted actor already dead in the scheduling round during which the turn_off simcall was issued?");
    REQUIRE(write_done);

    END_SECTION;
  }

  BEGIN_SECTION("asynchronous read stopped in middle")
  {
    XBT_INFO("Launch an read(5s), and restart its disk after 2 secs");
    auto disk = all_hosts[0]->get_disks().front();

    simgrid::s4u::Actor::create("io_read5_restarted", all_hosts[0], [disk]() {
      assert_exit(true, 2);
      auto read = disk->read_async(500000000);
      disk->turn_off();
      simgrid::s4u::this_actor::sleep_for(2);
      REQUIRE_STORAGE_FAILURE({read->wait();});
      disk->turn_on();
    });

    END_SECTION;
  }

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}
