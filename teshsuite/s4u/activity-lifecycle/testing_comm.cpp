/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

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

static void test_link_off_helper(double delay)
{
  const double start = simgrid::s4u::Engine::get_clock();

  simgrid::s4u::ActorPtr receiver = simgrid::s4u::Actor::create("receiver", all_hosts[1], [&start]() {
    assert_exit(true, 9);
    double milestone[5] = {0.5, 3.5, 4.5, 7.5, 9.0};
    for (double& m : milestone)
      m += start;
    for (int i = 0; i < 4; i++) {
      simgrid::s4u::this_actor::sleep_until(milestone[i]);
      REQUIRE_NETWORK_FAILURE({
        INFO("get(" << ('A' + i) << ")");
        simgrid::s4u::Mailbox::by_name("mb")->get();
      });
    }
    simgrid::s4u::this_actor::sleep_until(milestone[4]);
  });

  simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[2], [&start]() {
    assert_exit(true, 9);
    int data            = 42;
    double milestone[5] = {1.5, 2.5, 5.5, 6.5, 9.0};
    for (double& m : milestone)
      m += start;
    for (int i = 0; i < 2; i++) {
      simgrid::s4u::this_actor::sleep_until(milestone[i]);
      XBT_VERB("dsend(%c)", 'A' + i);
      simgrid::s4u::Mailbox::by_name("mb")->put_init(&data, 100000)->detach();
    }
    for (int i = 2; i < 4; i++) {
      simgrid::s4u::this_actor::sleep_until(milestone[i]);
      REQUIRE_NETWORK_FAILURE({
        INFO("put(" << ('A' + i) << ")");
        simgrid::s4u::Mailbox::by_name("mb")->put(&data, 100000);
      });
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
      REQUIRE_NETWORK_FAILURE({
        void* payload = simgrid::s4u::Mailbox::by_name("mb")->get();
        xbt_free(payload);
      });
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
            // Shouldn't get in here after the on_exit function
            in_catch_before_on_exit = not in_on_exit;
            in_catch_after_on_exit  = in_on_exit;
          }
          returned_from_main = true;
        });

    receiver->on_exit([&in_on_exit](bool) { in_on_exit = true; });

    simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[2], [&send_done]() {
      assert_exit(true, 1);
      int data = 42;
      REQUIRE_NETWORK_FAILURE(simgrid::s4u::Mailbox::by_name("mb")->put(&data, 100000));
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
      simgrid::s4u::CommPtr comm = simgrid::s4u::Mailbox::by_name("mb")->get_async((void**)&data);
      std::vector<simgrid::s4u::CommPtr> pending_comms = {comm};
      REQUIRE_NETWORK_FAILURE(simgrid::s4u::Comm::wait_any(&pending_comms));
    });

    simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[2], []() {
      assert_exit(true, 2);
      int data = 42;
      REQUIRE_NETWORK_FAILURE(simgrid::s4u::Mailbox::by_name("mb")->put(&data, 100000));
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
