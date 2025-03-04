/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "teshsuite/catch_simgrid.hpp"
#include <array>

static void test_link_off_helper(double delay)
{
  const double start = simgrid::s4u::Engine::get_clock();

  simgrid::s4u::ActorPtr receiver =
      simgrid::s4u::Engine::get_instance()->add_actor("receiver", all_hosts[1], [&start]() {
        assert_exit(true, 9);
        std::array<double, 5> milestone{{0.5, 3.5, 4.5, 7.5, 9.0}};
        for (double& m : milestone)
          m += start;
        for (int i = 0; i < 4; i++) {
          simgrid::s4u::this_actor::sleep_until(milestone[i]);
          REQUIRE_NETWORK_FAILURE({
            INFO("get(" << ('A' + i) << ")");
            simgrid::s4u::Mailbox::by_name("mb")->get<int>();
          });
        }
        simgrid::s4u::this_actor::sleep_until(milestone[4]);
      });

  simgrid::s4u::ActorPtr sender = simgrid::s4u::Engine::get_instance()->add_actor("sender", all_hosts[2], [&start]() {
    assert_exit(true, 9);
    int data            = 42;
    std::array<double, 5> milestone{{1.5, 2.5, 5.5, 6.5, 9.0}};
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

static simgrid::s4u::ActorPtr sender_basic(bool& ending_boolean, bool expected_success, double duration,
                                           double delay = -1.0)
{
  return simgrid::s4u::Engine::get_instance()->add_actor(
      "sender", all_hosts[1], [&ending_boolean, expected_success, duration, delay]() {
        assert_exit(expected_success, duration);
        // Encapsulate the payload in a std::unique_ptr so that it is correctly free'd if/when the sender is killed
        // during its communication (thanks to RAII).  The pointer is then released when the communication is over.
        std::unique_ptr<char, decltype(&xbt_free_f)> payload(xbt_strdup("toto"), &xbt_free_f);
        if (delay > 0.0) {
          simgrid::s4u::this_actor::sleep_for(delay / 2.0);
          auto comm = simgrid::s4u::Mailbox::by_name("mb")->put_init(payload.get(), 5000);
          simgrid::s4u::this_actor::sleep_for(delay / 2.0);
          comm->wait();
        } else {
          simgrid::s4u::Mailbox::by_name("mb")->put(payload.get(), 5000);
        }
        payload.release();
        ending_boolean = true;
      });
}
static simgrid::s4u::ActorPtr receiver_basic(bool& ending_boolean, bool expected_success, double duration,
                                             double delay = -1.0)
{
  return simgrid::s4u::Engine::get_instance()->add_actor(
      "receiver", all_hosts[2], [&ending_boolean, expected_success, duration, delay]() {
        assert_exit(expected_success, duration);
        char* payload;
        if (delay > 0.0) {
          simgrid::s4u::this_actor::sleep_for(delay / 2.0);
          auto comm = simgrid::s4u::Mailbox::by_name("mb")->get_init()->set_dst_data(reinterpret_cast<void**>(&payload),
                                                                                     sizeof(void*));
          simgrid::s4u::this_actor::sleep_for(delay / 2.0);
          comm->wait();
        } else {
          payload = simgrid::s4u::Mailbox::by_name("mb")->get<char>();
        }
        xbt_free(payload);
        ending_boolean = true;
      });
}
static simgrid::s4u::ActorPtr sender_dtach(bool& ending_boolean, bool expected_success, double duration)
{
  return simgrid::s4u::Engine::get_instance()->add_actor(
      "sender", all_hosts[1], [&ending_boolean, expected_success, duration]() {
        assert_exit(expected_success, duration);
        char* payload = xbt_strdup("toto");
        simgrid::s4u::Mailbox::by_name("mb")->put_init(payload, 1000)->detach();
        ending_boolean = true;
      });
}

TEST_CASE("Activity lifecycle: comm activities")
{
  XBT_INFO("#####[ launch next \"comm\" test ]#####");

  BEGIN_SECTION("comm")
  {
    XBT_INFO("Launch a communication");
    bool send_done = false;
    bool recv_done = false;

    sender_basic(send_done, true, 5);
    receiver_basic(recv_done, true, 5);

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Sender or receiver killed somehow. It shouldn't");
    REQUIRE(send_done);
    REQUIRE(recv_done);

    END_SECTION;
  }

  BEGIN_SECTION("comm (delayed send)")
  {
    XBT_INFO("Launch a communication with a delay for the send");
    bool send_done = false;
    bool recv_done = false;

    sender_basic(send_done, true, 6, 1); // cover Comm::send
    receiver_basic(recv_done, true, 6);

    simgrid::s4u::this_actor::sleep_for(9);
    INFO("Sender or receiver killed somehow. It shouldn't");
    REQUIRE(send_done);
    REQUIRE(recv_done);

    END_SECTION;
  }

  BEGIN_SECTION("comm (delayed recv)")
  {
    XBT_INFO("Launch a communication with a delay for the recv");
    bool send_done = false;
    bool recv_done = false;

    sender_basic(send_done, true, 6);
    receiver_basic(recv_done, true, 6, 1); // cover Comm::recv

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

    sender_dtach(dsend_done, true, 0);
    simgrid::s4u::this_actor::sleep_for(2);
    receiver_basic(recv_done, true, 1);

    // Sleep long enough to let the test ends by itself. 1 + precision_timing should be enough.
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

    receiver_basic(recv_done, true, 3);
    simgrid::s4u::this_actor::sleep_for(2);
    sender_dtach(dsend_done, true, 0);

    // Sleep long enough to let the test ends by itself. 3 + precision_timing should be enough.
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

    simgrid::s4u::ActorPtr sender = sender_basic(send_done, false, 2);

    simgrid::s4u::Engine::get_instance()->add_actor("receiver", all_hosts[2], [&recv_done]() {
      assert_exit(true, 2);
      REQUIRE_NETWORK_FAILURE({
        char* payload = simgrid::s4u::Mailbox::by_name("mb")->get<char>();
        xbt_free(payload);
      });
      recv_done = true;
    });

    simgrid::s4u::this_actor::sleep_for(2);
    sender->kill();
    // let the test ends by itself. waiting for precision_timing should be enough.
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

    simgrid::s4u::ActorPtr receiver = simgrid::s4u::Engine::get_instance()->add_actor(
        "receiver", all_hosts[1],
        [&in_on_exit, &returned_from_main, &in_catch_before_on_exit, &in_catch_after_on_exit]() {
          assert_exit(false, 1);
          try {
            simgrid::s4u::Mailbox::by_name("mb")->get<int>();
          } catch (simgrid::NetworkFailureException const&) {
            // Shouldn't get in here after the on_exit function
            in_catch_before_on_exit = not in_on_exit;
            in_catch_after_on_exit  = in_on_exit;
          }
          returned_from_main = true;
        });

    receiver->on_exit([&in_on_exit](bool) { in_on_exit = true; });

    simgrid::s4u::ActorPtr sender =
        simgrid::s4u::Engine::get_instance()->add_actor("sender", all_hosts[2], [&send_done]() {
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
    simgrid::s4u::ActorPtr receiver = simgrid::s4u::Engine::get_instance()->add_actor("receiver", all_hosts[1], []() {
      assert_exit(true, 2);
      int* data;
      simgrid::s4u::CommPtr comm = simgrid::s4u::Mailbox::by_name("mb")->get_async<int>(&data);
      simgrid::s4u::ActivitySet pending_comms({comm});
      REQUIRE_NETWORK_FAILURE(pending_comms.wait_any());
    });

    simgrid::s4u::ActorPtr sender = simgrid::s4u::Engine::get_instance()->add_actor("sender", all_hosts[2], []() {
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
