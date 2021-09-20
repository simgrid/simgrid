/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>
#include <iostream>
#include <simgrid/s4u.hpp>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(meh, "meh");

static void worker()
{
  auto mbox  = simgrid::s4u::Mailbox::by_name("meh");
  int input1 = 42;
  int input2 = 51;

  XBT_INFO("Sending and receiving %d and %d asynchronously", input1, input2);

  auto put1 = mbox->put_async(&input1, 1000 * 1000 * 500);
  auto put2 = mbox->put_async(&input2, 1000 * 1000 * 1000);

  int* out1;
  auto get1 = mbox->get_async<int>(&out1);

  int* out2;
  auto get2 = mbox->get_async<int>(&out2);

  XBT_INFO("All comms have started");
  std::vector<simgrid::s4u::CommPtr> comms = {put1, put2, get1, get2};

  while (not comms.empty()) {
    size_t index = simgrid::s4u::Comm::wait_all_for(comms, 0.5);
    if (index < comms.size())
      XBT_INFO("wait_all_for: Timeout reached");
    XBT_INFO("wait_all_for: %zu comms finished (#comms=%zu)", index, comms.size());
    comms.erase(comms.begin(), comms.begin() + index);
  }

  XBT_INFO("All comms have finished");
  XBT_INFO("Got %d and %d", *out1, *out2);
}

int main(int argc, char* argv[])

{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("worker", e.host_by_name("Tremblay"), worker);
  e.run();
  return 0;
}
