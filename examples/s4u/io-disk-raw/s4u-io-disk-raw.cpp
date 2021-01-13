/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <string>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_CATEGORY(disk, "Messages specific for this simulation");

static void host()
{
  /* - Display information on the disks mounted by the current host */
  XBT_INFO("*** Storage info on %s ***", simgrid::s4u::Host::current()->get_cname());

  /* - Retrieve all disks from current host */
  std::vector<simgrid::s4u::Disk*> const& disk_list = simgrid::s4u::Host::current()->get_disks();

  /* - For each disk mounted on host, display disk name and mount point */
  for (auto const& disk : disk_list)
    XBT_INFO("Disk name: %s (read: %.0f B/s -- write: %.0f B/s ", disk->get_cname(), disk->get_read_bandwidth(),
             disk->get_write_bandwidth());

  /* - Write 400,000 bytes on Disk1 */
  simgrid::s4u::Disk* disk = disk_list.front();
  sg_size_t write          = disk->write(400000);
  XBT_INFO("Wrote %llu bytes on '%s'", write, disk->get_cname());

  /*  - Now read 200,000 bytes */
  sg_size_t read = disk->read(200000);
  XBT_INFO("Read %llu bytes on '%s'", read, disk->get_cname());

  /* - Attach some user data to disk1 */
  XBT_INFO("*** Get/set data for storage element: Disk1 ***");

  const auto* data = static_cast<std::string*>(disk->get_data());

  XBT_INFO("Get storage data: '%s'", data ? data->c_str() : "No user data");

  disk->set_data(new std::string("Some user data"));
  data = static_cast<std::string*>(disk->get_data());
  XBT_INFO("Set and get data: '%s'", data->c_str());
  delete data;
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  /* - Display Host properties */
  for (auto h : e.get_all_hosts()) {
    XBT_INFO("*** %s properties ****", h->get_cname());
    for (auto const& kv : *h->get_properties())
      XBT_INFO("  %s -> %s", kv.first.c_str(), kv.second.c_str());
  }

  simgrid::s4u::Actor::create("", simgrid::s4u::Host::by_name("bob"), host);

  e.run();
  XBT_INFO("Simulated time: %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
