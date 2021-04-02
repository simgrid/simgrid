/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u test");

static void host()
{
  const simgrid::s4u::Disk* disk = simgrid::s4u::this_actor::get_host()->get_disks().front(); // Disk1
  aid_t id                       = simgrid::s4u::this_actor::get_pid();
  XBT_INFO("process %ld is writing!", id);
  disk->write(4000000);
  XBT_INFO("process %ld goes to sleep for %ld seconds", id, id);
  simgrid::s4u::this_actor::sleep_for(static_cast<double>(id));
  XBT_INFO("process %ld is writing again!", id);
  disk->write(4000000);
  XBT_INFO("process %ld goes to sleep for %ld seconds", id, 6 - id);
  simgrid::s4u::this_actor::sleep_for(static_cast<double>(6 - id));
  XBT_INFO("process %ld is reading!", id);
  disk->read(4000000);
  XBT_INFO("process %ld goes to sleep for %ld seconds", id, id);
  simgrid::s4u::this_actor::sleep_for(static_cast<double>(id));
  XBT_INFO("process %ld is reading again!", id);
  disk->read(4000000);
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  for (int i = 0; i < 5; i++)
    simgrid::s4u::Actor::create("host", simgrid::s4u::Host::by_name("bob"), host);

  e.run();
  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
