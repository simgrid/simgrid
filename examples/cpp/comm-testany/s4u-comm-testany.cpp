/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_testany, "Messages specific for this s4u example");

static void rank0()
{
  sg4::Mailbox* mbox = sg4::Mailbox::by_name(std::string("rank0"));
  std::string* msg1;
  std::string* msg2;
  std::string* msg3;

  XBT_INFO("Post my asynchronous receives");
  auto comm1                              = mbox->get_async(&msg1);
  auto comm2                              = mbox->get_async(&msg2);
  auto comm3                              = mbox->get_async(&msg3);
  std::vector<sg4::CommPtr> pending_comms = {comm1, comm2, comm3};

  XBT_INFO("Send some data to rank-1");
  for (int i = 0; i < 3; i++)
    sg4::Mailbox::by_name(std::string("rank1"))->put(new int(i), 1);

  XBT_INFO("Test for completed comms");
  while (not pending_comms.empty()) {
    ssize_t flag = sg4::Comm::test_any(pending_comms);
    if (flag != -1) {
      pending_comms.erase(pending_comms.begin() + flag);
      XBT_INFO("Remove a pending comm.");
    } else // nothing matches, wait for a little bit
      sg4::this_actor::sleep_for(0.1);
  }
  XBT_INFO("Last comm is complete");
  delete msg1;
  delete msg2;
  delete msg3;
}

static void rank1()
{
  sg4::Mailbox* rank0_mbox = sg4::Mailbox::by_name(std::string("rank0"));
  sg4::Mailbox* rank1_mbox = sg4::Mailbox::by_name(std::string("rank1"));

  for (int i = 0; i < 3; i++) {
    auto res = rank1_mbox->get_unique<int>();
    XBT_INFO("Received %d", *res);
    std::string msg_content = std::string("Message ") + std::to_string(i);
    auto* payload           = new std::string(msg_content);
    XBT_INFO("Send '%s'", msg_content.c_str());
    rank0_mbox->put(payload, 1e6);
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("rank-0", e.host_by_name("Tremblay"), rank0);
  sg4::Actor::create("rank-1", e.host_by_name("Fafard"), rank1);

  e.run();

  return 0;
}
