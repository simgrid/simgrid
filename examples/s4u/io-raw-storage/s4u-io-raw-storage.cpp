/* Copyright (c) 2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <string>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_CATEGORY(storage, "Messages specific for this simulation");

static void host()
{
  /* - Display information on the disks mounted by the current host */
  XBT_INFO("*** Storage info on %s ***", simgrid::s4u::Host::current()->getCname());

  /* - Retrieve all mount points of current host */
  std::unordered_map<std::string, simgrid::s4u::Storage*> const& storage_list =
      simgrid::s4u::Host::current()->getMountedStorages();

  /* - For each disk mounted on host, display disk name and mount point */
  for (auto const& kv : storage_list)
    XBT_INFO("Storage name: %s, mount name: %s", kv.second->getCname(), kv.first.c_str());

  /* - Write 200,000 bytes on Disk4 */
  simgrid::s4u::Storage* storage = simgrid::s4u::Storage::byName("Disk4");
  sg_size_t write                = storage->write(200000);
  XBT_INFO("Wrote %llu bytes on 'Disk4'", write);

  /*  - Now read 200,000 bytes */
  sg_size_t read = storage->read(200000);
  XBT_INFO("Read %llu bytes on 'Disk4'", read);

  /* - Attach some user data to disk1 */
  XBT_INFO("*** Get/set data for storage element: Disk4 ***");

  char* data = static_cast<char*>(storage->getUserdata());

  XBT_INFO("Get storage data: '%s'", data);

  storage->setUserdata(xbt_strdup("Some user data"));
  data = static_cast<char*>(storage->getUserdata());
  XBT_INFO("Set and get data: '%s'", data);
  xbt_free(data);
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("", simgrid::s4u::Host::by_name("denise"), host);

  e.run();
  XBT_INFO("Simulated time: %g", simgrid::s4u::Engine::getClock());

  return 0;
}
