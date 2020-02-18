/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define CATCH_CONFIG_RUNNER // we supply our own main()

#include "../../src/include/catch.hpp"

#include <simgrid/s4u.hpp>
#include <xbt/config.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

std::vector<simgrid::s4u::Host*> all_hosts;

/* Helper function easing the testing of actor's ending condition */
static void assert_exit(bool exp_success, double duration)
{
  double expected_time = simgrid::s4u::Engine::get_clock() + duration;
  simgrid::s4u::this_actor::on_exit([exp_success, expected_time](bool got_failed) {
    INFO("Check exit status. Expected: " << exp_success);
    REQUIRE(exp_success == not got_failed);
    INFO("Check date at exit. Expected: " << expected_time);
    REQUIRE(simgrid::s4u::Engine::get_clock() == Approx(expected_time));
    XBT_VERB("Checks on exit successful");
  });
}

/* Helper function in charge of doing some sanity checks after each test */
static void assert_cleanup()
{
  /* Check that no actor remain (but on host[0], where main_dispatcher lives */
  for (unsigned int i = 0; i < all_hosts.size(); i++) {
    std::vector<simgrid::s4u::ActorPtr> all_actors = all_hosts[i]->get_all_actors();
    unsigned int expected_count = (i == 0) ? 1 : 0; // host[0] contains main_dispatcher, all other are empty
    if (all_actors.size() != expected_count) {
      INFO("Host " << all_hosts[i]->get_cname() << " contains " << all_actors.size() << " actors but " << expected_count
                   << " are expected (i=" << i << "). Existing actors: ");
      for (auto act : all_actors)
        UNSCOPED_INFO(" - " << act->get_cname());
      FAIL("This is wrong");
    }
  }
  // TODO: Check that all LMM are empty
}

/**
 ** Each tests
 **/

/* We need an extra actor here, so that it can sleep until the end of each test */
#define BEGIN_SECTION(descr) SECTION(descr) { simgrid::s4u::Actor::create(descr, all_hosts[0], []()
#define END_SECTION })

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

TEST_CASE("Activity lifecycle: exec activities")
{
  XBT_INFO("#####[ launch next \"exec\" test ]#####");

  BEGIN_SECTION("exec")
  {
    XBT_INFO("Launch a execute(5s), and let it proceed");
    bool global = false;

    simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], [&global]() {
      assert_exit(true, 5.);
      simgrid::s4u::this_actor::execute(500000000);
      global = true;
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Did the forked actor modify the global after sleeping, or was it killed before?");
    REQUIRE(global);

    END_SECTION;
  }

  BEGIN_SECTION("exec killed at start")
  {
    XBT_INFO("Launch a execute(5s), and kill it right after start");
    simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5_killed", all_hosts[1], []() {
      assert_exit(false, 0);
      simgrid::s4u::this_actor::execute(500000000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    exec5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("exec killed in middle")
  {
    XBT_INFO("Launch a execute(5s), and kill it after 2 secs");
    simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5_killed", all_hosts[1], []() {
      assert_exit(false, 2);
      simgrid::s4u::this_actor::execute(500000000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    exec5->kill();

    END_SECTION;
  }

  BEGIN_SECTION("exec restarted at start")
  {
    XBT_INFO("Launch a execute(5s), and restart its host right after start");
    simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5_restarted", all_hosts[1], []() {
      assert_exit(false, 0);
      simgrid::s4u::this_actor::execute(500000000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::yield();
    exec5->get_host()->turn_off();
    exec5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("exec restarted in middle")
  {
    XBT_INFO("Launch a execute(5s), and restart its host after 2 secs");
    simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5_restarted", all_hosts[1], []() {
      assert_exit(false, 2);
      simgrid::s4u::this_actor::execute(500000000);
      FAIL("I should be dead now");
    });

    simgrid::s4u::this_actor::sleep_for(2);
    exec5->get_host()->turn_off();
    exec5->get_host()->turn_on();

    END_SECTION;
  }

  BEGIN_SECTION("exec restarted at end")
  {
    XBT_INFO("Launch a execute(5s), and restart its host right when it stops");
    bool execution_done = false;

    simgrid::s4u::Actor::create("exec5_restarted", all_hosts[1], [&execution_done]() {
      assert_exit(true, 5);
      simgrid::s4u::this_actor::execute(500000000);
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

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}

TEST_CASE("Activity lifecycle: comm activities")
{
  XBT_INFO("#####[ launch next \"comm\" test ]#####");

  BEGIN_SECTION("comm")
  {
    XBT_INFO("Launch a communication");
    bool send_done = false;
    bool recv_done = false;

    simgrid::s4u::Actor::create("sender", all_hosts[1], [&send_done]() {
      assert_exit(true, 5);
      char* payload = xbt_strdup("toto");
      simgrid::s4u::Mailbox::by_name("mb")->put(payload, 5000);
      send_done = true;
    });

    simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
      assert_exit(true, 5);
      void* payload = simgrid::s4u::Mailbox::by_name("mb")->get();
      xbt_free(payload);
      recv_done = true;
    });

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Sender or receiver killed somehow. It shouldn't");
    REQUIRE(send_done);
    REQUIRE(recv_done);

    END_SECTION;
  }

  BEGIN_SECTION("comm dsend and quit (put before get)")
  {
    XBT_INFO("Launch a detached communication and end right after");
    bool dsend_done = false;
    bool recv_done  = false;

    simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[1], [&dsend_done]() {
      assert_exit(true, 0);
      char* payload = xbt_strdup("toto");
      simgrid::s4u::Mailbox::by_name("mb")->put_init(payload, 1000)->detach();
      dsend_done = true;
    });

    simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
      assert_exit(true, 3);
      simgrid::s4u::this_actor::sleep_for(2);
      void* payload = simgrid::s4u::Mailbox::by_name("mb")->get();
      xbt_free(payload);
      recv_done = true;
    });

    // Sleep long enough to let the test ends by itself. 3 + surf_precision should be enough.
    simgrid::s4u::this_actor::sleep_for(4);
    INFO("Sender or receiver killed somehow. It shouldn't");
    REQUIRE(dsend_done);
    REQUIRE(recv_done);

    END_SECTION;
  }

  BEGIN_SECTION("comm dsend and quit (get before put)")
  {
    XBT_INFO("Launch a detached communication and end right after");
    bool dsend_done = false;
    bool recv_done  = false;

    simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[1], [&dsend_done]() {
      assert_exit(true, 2);
      char* payload = xbt_strdup("toto");
      simgrid::s4u::this_actor::sleep_for(2);
      simgrid::s4u::Mailbox::by_name("mb")->put_init(payload, 1000)->detach();
      dsend_done = true;
    });

    simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
      assert_exit(true, 3);
      void* payload = simgrid::s4u::Mailbox::by_name("mb")->get();
      xbt_free(payload);
      recv_done = true;
    });

    // Sleep long enough to let the test ends by itself. 3 + surf_precision should be enough.
    simgrid::s4u::this_actor::sleep_for(4);
    INFO("Sender or receiver killed somehow. It shouldn't");
    REQUIRE(dsend_done);
    REQUIRE(recv_done);

    END_SECTION;
  }

  BEGIN_SECTION("comm kill sender")
  {
    XBT_INFO("Launch a communication and kill the sender");
    bool send_done = false;
    bool recv_done = false;

    simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[1], [&send_done]() {
      assert_exit(false, 2);
      // Encapsulate the payload in a std::unique_ptr so that it is correctly free'd when the sender is killed during
      // its communication (thanks to RAII).  The pointer is then released when the communication is over.
      std::unique_ptr<char, decltype(&xbt_free_f)> payload(xbt_strdup("toto"), &xbt_free_f);
      simgrid::s4u::Mailbox::by_name("mb")->put(payload.get(), 5000);
      payload.release();
      send_done = true;
    });

    simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
      assert_exit(true, 2);
      void* payload = nullptr;
      REQUIRE_THROWS_AS(payload = simgrid::s4u::Mailbox::by_name("mb")->get(), simgrid::NetworkFailureException);
      xbt_free(payload);
      recv_done = true;
    });

    simgrid::s4u::this_actor::sleep_for(2);
    sender->kill();
    // let the test ends by itself. waiting for surf_precision should be enough.
    simgrid::s4u::this_actor::sleep_for(0.00001);

    INFO("Sender was not killed properly or receiver killed somehow. It shouldn't");
    REQUIRE(not send_done);
    REQUIRE(recv_done);

    END_SECTION;
  }

  BEGIN_SECTION("comm recv and kill")
  {
    XBT_INFO("Launch an actor that waits on a recv, kill its host");
    bool in_on_exit              = false;
    bool returned_from_main      = false;
    bool in_catch_before_on_exit = false;
    bool in_catch_after_on_exit  = false;
    bool send_done               = false;

    simgrid::s4u::ActorPtr receiver =
        simgrid::s4u::Actor::create("receiver", all_hosts[1], [&in_on_exit, &returned_from_main,
                                                               &in_catch_before_on_exit, &in_catch_after_on_exit]() {
          assert_exit(false, 1);
          try {
            simgrid::s4u::Mailbox::by_name("mb")->get();
          } catch (simgrid::NetworkFailureException const&) {
            // Shouldn't get in here
            in_catch_before_on_exit = not in_on_exit;
            in_catch_after_on_exit  = in_on_exit;
          }
          returned_from_main = true;
        });

    receiver->on_exit([&in_on_exit](bool) { in_on_exit = true; });

    simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[2], [&send_done]() {
      assert_exit(true, 1);
      int data = 42;
      REQUIRE_THROWS_AS(simgrid::s4u::Mailbox::by_name("mb")->put(&data, 100000), simgrid::NetworkFailureException);
      send_done = true;
    });

    simgrid::s4u::this_actor::sleep_for(1);
    receiver->get_host()->turn_off();

    // Note: If we don't sleep here, we don't "see" the bug
    simgrid::s4u::this_actor::sleep_for(1);

    INFO("Receiver's on_exit function was never called");
    REQUIRE(in_on_exit);
    INFO("or receiver mistakenly went to catch clause (before the on_exit function was called)");
    REQUIRE(not in_catch_before_on_exit);
    INFO("or receiver mistakenly went to catch clause (after the on_exit function was called)");
    REQUIRE(not in_catch_after_on_exit);
    INFO("or receiver returned from main normally even though its host was killed");
    REQUIRE(not returned_from_main);
    INFO("or sender killed somehow, and it shouldn't");
    REQUIRE(send_done);
    receiver->get_host()->turn_on();

    END_SECTION;
  }

  static auto test_link_off_helper = [](double delay) {
    const double start = simgrid::s4u::Engine::get_clock();

    simgrid::s4u::ActorPtr receiver = simgrid::s4u::Actor::create("receiver", all_hosts[1], [&start]() {
      assert_exit(true, 9);
      double milestone[5] = {0.5, 3.5, 4.5, 7.5, 9.0};
      for (int i = 0; i < 5; i++)
        milestone[i] += start;
      for (int i = 0; i < 4; i++) {
        simgrid::s4u::this_actor::sleep_until(milestone[i]);
        try {
          XBT_VERB("get(%c)", 'A' + i);
          simgrid::s4u::Mailbox::by_name("mb")->get();
          return;
        } catch (simgrid::NetworkFailureException const&) {
          XBT_VERB("got expected NetworkFailureException");
        }
      }
      simgrid::s4u::this_actor::sleep_until(milestone[4]);
    });

    simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[2], [&start]() {
      assert_exit(true, 9);
      int data            = 42;
      double milestone[5] = {1.5, 2.5, 5.5, 6.5, 9.0};
      for (int i = 0; i < 5; i++)
        milestone[i] += start;
      for (int i = 0; i < 2; i++) {
        simgrid::s4u::this_actor::sleep_until(milestone[i]);
        XBT_VERB("dsend(%c)", 'A' + i);
        simgrid::s4u::Mailbox::by_name("mb")->put_init(&data, 100000)->detach();
      }
      for (int i = 2; i < 4; i++) {
        simgrid::s4u::this_actor::sleep_until(milestone[i]);
        try {
          XBT_VERB("put(%c)", 'A' + i);
          simgrid::s4u::Mailbox::by_name("mb")->put(&data, 100000);
          return;
        } catch (simgrid::NetworkFailureException const&) {
          XBT_VERB("got expected NetworkFailureException");
        }
      }
      simgrid::s4u::this_actor::sleep_until(milestone[4]);
    });

    for (int i = 0; i < 4; i++) {
      XBT_VERB("##### %d / 4 #####", i + 1);
      simgrid::s4u::this_actor::sleep_for(delay);
      XBT_VERB("link off");
      simgrid::s4u::Link::by_name("link1")->turn_off();
      simgrid::s4u::this_actor::sleep_for(2.0 - delay);
      XBT_VERB("link on");
      simgrid::s4u::Link::by_name("link1")->turn_on();
    }
    simgrid::s4u::this_actor::sleep_for(1.5);
  };

  BEGIN_SECTION("comm turn link off before send/recv")
  {
    XBT_INFO("try to communicate with communicating link turned off before start");
    test_link_off_helper(0.0);

    END_SECTION;
  }

  BEGIN_SECTION("comm turn link off between send/recv")
  {
    XBT_INFO("try to communicate with communicating link turned off between send and receive");
    test_link_off_helper(1.0);

    END_SECTION;
  }

  BEGIN_SECTION("comm turn link off during transfer")
  {
    XBT_INFO("try to communicate with communicating link turned off during transfer");
    test_link_off_helper(2.0);

    END_SECTION;
  }

  BEGIN_SECTION("comm turn link off during wait_any")
  {
    XBT_INFO("try to communicate with communicating link turned off during wait_any");
    simgrid::s4u::ActorPtr receiver = simgrid::s4u::Actor::create("receiver", all_hosts[1], []() {
      assert_exit(true, 2);
      int* data;
      std::vector<simgrid::s4u::CommPtr> pending_comms;
      simgrid::s4u::CommPtr comm = simgrid::s4u::Mailbox::by_name("mb")->get_async((void**)&data);
      pending_comms.push_back(comm);
      REQUIRE_THROWS_AS(simgrid::s4u::Comm::wait_any(&pending_comms), simgrid::NetworkFailureException);
    });

    simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[2], []() {
      assert_exit(true, 2);
      int data                                  = 42;
      REQUIRE_THROWS_AS(simgrid::s4u::Mailbox::by_name("mb")->put(&data, 100000), simgrid::NetworkFailureException);
    });

    simgrid::s4u::this_actor::sleep_for(2.0);
    XBT_VERB("link off");
    simgrid::s4u::Link::by_name("link1")->turn_off();
    simgrid::s4u::this_actor::sleep_for(2.0);
    XBT_VERB("link on");
    simgrid::s4u::Link::by_name("link1")->turn_on();

    END_SECTION;
  }

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}

int main(int argc, char* argv[])
{
  simgrid::config::set_value("help-nostop", true);
  simgrid::s4u::Engine e(&argc, argv);

  std::string platf;
  if (argc > 1) {
    platf   = argv[1];
    argv[1] = argv[0];
    argv++;
    argc--;
  } else {
    XBT_WARN("No platform file provided. Using './testing_platform.xml'");
    platf = "./testing_platform.xml";
  }
  e.load_platform(platf);

  int status = 42;
  all_hosts = e.get_all_hosts();
  simgrid::s4u::Actor::create("main_dispatcher", all_hosts[0],
                              [&argc, &argv, &status]() { status = Catch::Session().run(argc, argv); });

  e.run();
  XBT_INFO("Simulation done");
  return status;
}
