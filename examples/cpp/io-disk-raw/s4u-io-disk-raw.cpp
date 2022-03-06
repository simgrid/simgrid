/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <string>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_CATEGORY(disk_test, "Messages specific for this simulation");
namespace sg4 = simgrid::s4u;

static void host()
{
  /* -Add an extra disk in a programmatic way */
  sg4::Host::current()->create_disk("Disk3", /*read bandwidth*/ 9.6e7, /*write bandwidth*/ 6.4e7)->seal();

  /* - Display information on the disks mounted by the current host */
  XBT_INFO("*** Storage info on %s ***", sg4::Host::current()->get_cname());

  /* - Retrieve all disks from current host */
  std::vector<sg4::Disk*> const& disk_list = sg4::Host::current()->get_disks();

  /* - For each disk mounted on host, display disk name and mount point */
  for (auto const& disk : disk_list)
    XBT_INFO("Disk name: %s (read: %.0f B/s -- write: %.0f B/s", disk->get_cname(), disk->get_read_bandwidth(),
             disk->get_write_bandwidth());

  /* - Write 400,000 bytes on Disk1 */
  sg4::Disk* disk          = disk_list.front();
  sg_size_t write          = disk->write(400000);
  XBT_INFO("Wrote %llu bytes on '%s'", write, disk->get_cname());

  /*  - Now read 200,000 bytes */
  sg_size_t read = disk->read(200000);
  XBT_INFO("Read %llu bytes on '%s'", read, disk->get_cname());

  /* - Write 800,000 bytes on Disk3 */
  const sg4::Disk* disk3          = disk_list.back();
  sg_size_t write_on_disk3        = disk3->write(800000);
  XBT_INFO("Wrote %llu bytes on '%s'", write_on_disk3, disk3->get_cname());

  /* - Attach some user data to disk1 */
  XBT_INFO("*** Get/set data for storage element: Disk1 ***");

  const auto* data = disk->get_data<std::string>();

  XBT_INFO("Get storage data: '%s'", data ? data->c_str() : "No user data");

  disk->set_data(new std::string("Some user data"));
  data = disk->get_data<std::string>();
  XBT_INFO("Set and get data: '%s'", data->c_str());
  delete data;
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  /* - Display Host properties */
  for (auto h : e.get_all_hosts()) {
    XBT_INFO("*** %s properties ****", h->get_cname());
    for (auto const& kv : *h->get_properties())
      XBT_INFO("  %s -> %s", kv.first.c_str(), kv.second.c_str());
  }

  sg4::Actor::create("", e.host_by_name("bob"), host);

  e.run();
  XBT_INFO("Simulated time: %g", sg4::Engine::get_clock());

  return 0;
}
