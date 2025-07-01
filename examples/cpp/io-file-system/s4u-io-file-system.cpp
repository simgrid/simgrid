/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <vector>

#include "simgrid/plugins/file_system.h"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");
namespace sg4 = simgrid::s4u;

class MyHost {
public:
  void show_info(std::vector<sg4::Disk*> const& disks) const
  {
    XBT_INFO("Storage info on %s:", sg4::Host::current()->get_cname());

    for (auto const& d : disks) {
      // Retrieve disk's information
      XBT_INFO("    %s (%s) Used: %llu; Free: %llu; Total: %llu.", d->get_cname(), sg_disk_get_mount_point(d),
               sg_disk_get_size_used(d), sg_disk_get_size_free(d), sg_disk_get_size(d));
    }
  }

  void operator()() const
  {
    std::vector<sg4::Disk*> const& disks = sg4::Host::current()->get_disks();

    show_info(disks);

    // Open a non-existing file to create it
    std::string filename     = "/scratch/tmp/data.txt";
    auto* file               = sg4::File::open(filename, nullptr);

    sg_size_t write = file->write(200000); // Write 200,000 bytes
    XBT_INFO("Create a %llu bytes file named '%s' on /scratch", write, filename.c_str());

    // check that sizes have changed
    show_info(disks);

    // Now retrieve the size of created file and read it completely
    const sg_size_t file_size = file->size();
    file->seek(0);
    const sg_size_t read = file->read(file_size);
    XBT_INFO("Read %llu bytes on %s", read, filename.c_str());

    // Now write 100,000 bytes in tmp/data.txt
    write = file->write(100000); // Write 100,000 bytes
    XBT_INFO("Write %llu bytes on %s", write, filename.c_str());

    // Now rename file from ./tmp/data.txt to ./tmp/simgrid.readme
    std::string newpath = "/scratch/tmp/simgrid.readme";
    XBT_INFO("Move '%s' to '%s'", file->get_path(), newpath.c_str());
    file->move(newpath);

    // Test attaching some user data to the file
    file->set_data(new std::string("777"));
    auto file_data = file->get_unique_data<std::string>();
    XBT_INFO("User data attached to the file: %s", file_data->c_str());

    // Close the file
    file->close();

    show_info(disks);

    // Reopen the file and then unlink it
    file = sg4::File::open("/scratch/tmp/simgrid.readme", nullptr);
    XBT_INFO("Unlink file: '%s'", file->get_path());
    file->unlink();
    file->close(); // Unlinking the file on "disk" does not close the file and free the object

    show_info(disks);

    // Open another file on disk without a "content" property
    filename = "/lib/libc.so";
    file     = sg4::File::open(filename, nullptr);
    write    = file->write(4096); // Write 4 Kbytes
    XBT_INFO("Create a %llu bytes file named '%s' on /", write, filename.c_str());
    file->close();

    show_info(disks);
  }
};

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);
  e.host_by_name("bob")->add_actor("host", MyHost());
  e.run();

  return 0;
}
