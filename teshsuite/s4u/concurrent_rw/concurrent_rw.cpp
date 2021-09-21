/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u test");

namespace sg4 = simgrid::s4u;

static void host()
{
  const sg4::Disk* disk = sg4::this_actor::get_host()->get_disks().front(); // Disk1
  aid_t id              = sg4::this_actor::get_pid();
  XBT_INFO("actor %ld is writing!", id);
  disk->write(4000000);
  XBT_INFO("actor %ld goes to sleep for %ld seconds", id, id);
  sg4::this_actor::sleep_for(static_cast<double>(id));
  XBT_INFO("actor %ld is writing again!", id);
  disk->write(4000000);
  XBT_INFO("actor %ld goes to sleep for %ld seconds", id, 6 - id);
  sg4::this_actor::sleep_for(static_cast<double>(6 - id));
  XBT_INFO("actor %ld is reading!", id);
  disk->read(4000000);
  XBT_INFO("actor %ld goes to sleep for %ld seconds", id, id);
  sg4::this_actor::sleep_for(static_cast<double>(id));
  XBT_INFO("actor %ld is reading again!", id);
  disk->read(4000000);
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  for (int i = 0; i < 5; i++)
    sg4::Actor::create("host", e.host_by_name("bob"), host);

  e.run();
  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
