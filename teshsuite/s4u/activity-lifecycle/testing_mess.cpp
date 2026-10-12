/* Copyright (c) 2026. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "teshsuite/catch_simgrid.hpp"
namespace sg4 = simgrid::s4u;

TEST_CASE("Activity lifecycle: mess activities")
{
  XBT_INFO("#####[ launch next \"mess\" test ]#####");

  BEGIN_SECTION("mess")
  {
    XBT_INFO("Send a message through a message queue");
    bool send_done = false;
    bool recv_done = false;

    all_hosts[1]->add_actor("sender", [&send_done]() {
      assert_exit(true, 0);
      static int data = 42;
      sg4::MessageQueue::by_name("mq")->put(&data);
      send_done = true;
    });
    all_hosts[2]->add_actor("receiver", [&recv_done]() {
      assert_exit(true, 0);
      const int* data = sg4::MessageQueue::by_name("mq")->get<int>();
      REQUIRE(*data == 42);
      recv_done = true;
    });

    sg4::this_actor::sleep_for(1);
    INFO("Sender or receiver killed somehow. It shouldn't");
    REQUIRE(send_done);
    REQUIRE(recv_done);

    END_SECTION;
  }

  // In the following sections, the message is never matched and gets canceled by another actor (the one running the
  // section) at t=1. An actor that already waits for it at that time must be released with a CancelException.
  BEGIN_SECTION("mess cancel unmatched get while waiting")
  {
    XBT_INFO("Cancel a get that is not matched yet while the receiver waits for it");
    sg4::MessPtr mess;
    bool recv_done = false;
    all_hosts[1]->add_actor("receiver", [&mess, &recv_done]() {
      assert_exit(true, 1);
      int* data;
      mess = sg4::MessageQueue::by_name("mq")->get_async<int>(&data);
      REQUIRE_CANCEL(mess->wait());
      recv_done = true;
    });

    sg4::this_actor::sleep_for(1);
    mess->cancel();
    sg4::this_actor::sleep_for(1);
    INFO("The receiver was not released by the cancellation");
    REQUIRE(recv_done);

    END_SECTION;
  }

  BEGIN_SECTION("mess cancel unmatched get while waiting in wait_any")
  {
    XBT_INFO("Cancel a get that is not matched yet while the receiver waits for it in an ActivitySet");
    sg4::MessPtr mess;
    bool recv_done = false;
    all_hosts[1]->add_actor("receiver", [&mess, &recv_done]() {
      assert_exit(true, 1);
      int* data;
      mess = sg4::MessageQueue::by_name("mq")->get_async<int>(&data);
      sg4::ActivitySet pending_mess({mess});
      REQUIRE_CANCEL(pending_mess.wait_any());
      recv_done = true;
    });

    sg4::this_actor::sleep_for(1);
    mess->cancel();
    sg4::this_actor::sleep_for(1);
    INFO("The receiver was not released by the cancellation");
    REQUIRE(recv_done);

    END_SECTION;
  }

  BEGIN_SECTION("mess cancel unmatched put while waiting")
  {
    XBT_INFO("Cancel a put that is not matched yet while the sender waits for it");
    sg4::MessPtr mess;
    bool send_done = false;
    all_hosts[1]->add_actor("sender", [&mess, &send_done]() {
      assert_exit(true, 1);
      static int data = 42;
      mess            = sg4::MessageQueue::by_name("mq")->put_async(&data);
      REQUIRE_CANCEL(mess->wait());
      send_done = true;
    });

    sg4::this_actor::sleep_for(1);
    mess->cancel();
    sg4::this_actor::sleep_for(1);
    INFO("The sender was not released by the cancellation");
    REQUIRE(send_done);

    END_SECTION;
  }

  BEGIN_SECTION("mess wait after the cancellation of an unmatched get")
  {
    XBT_INFO("Wait for a get that was canceled before being matched");
    sg4::MessPtr mess;
    bool recv_done = false;
    all_hosts[1]->add_actor("receiver", [&mess, &recv_done]() {
      assert_exit(true, 2);
      int* data;
      mess = sg4::MessageQueue::by_name("mq")->get_async<int>(&data);
      sg4::this_actor::sleep_for(2);
      REQUIRE_CANCEL(mess->wait());
      recv_done = true;
    });

    sg4::this_actor::sleep_for(1);
    mess->cancel();
    sg4::this_actor::sleep_for(2);
    INFO("The receiver did not get a CancelException");
    REQUIRE(recv_done);

    END_SECTION;
  }

  sg4::this_actor::sleep_for(10);
  assert_cleanup();
}
