/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <unordered_map>

#include "simgrid/plugins/file_system.h"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

class MyHost {
public:
  void show_info(std::unordered_map<std::string, simgrid::s4u::Storage*> const& mounts)
  {
    XBT_INFO("Storage info on %s:", simgrid::s4u::Host::current()->get_cname());

    for (auto const& kv : mounts) {
      std::string mountpoint         = kv.first;
      simgrid::s4u::Storage* storage = kv.second;

      // Retrieve disk's information
      XBT_INFO("    %s (%s) Used: %llu; Free: %llu; Total: %llu.", storage->get_cname(), mountpoint.c_str(),
               sg_storage_get_size_used(storage), sg_storage_get_size_free(storage), sg_storage_get_size(storage));
    }
  }

  void operator()()
  {
    std::unordered_map<std::string, simgrid::s4u::Storage*> const& mounts =
        simgrid::s4u::Host::current()->get_mounted_storages();

    show_info(mounts);

    // Open an non-existing file to create it
    std::string filename     = "/home/tmp/data.txt";
    simgrid::s4u::File* file = new simgrid::s4u::File(filename, nullptr);

    sg_size_t write = file->write(200000); // Write 200,000 bytes
    XBT_INFO("Create a %llu bytes file named '%s' on /sd1", write, filename.c_str());

    // check that sizes have changed
    show_info(mounts);

    // Now retrieve the size of created file and read it completely
    const sg_size_t file_size = file->size();
    file->seek(0);
    const sg_size_t read = file->read(file_size);
    XBT_INFO("Read %llu bytes on %s", read, filename.c_str());

    // Now write 100,000 bytes in tmp/data.txt
    write = file->write(100000); // Write 100,000 bytes
    XBT_INFO("Write %llu bytes on %s", write, filename.c_str());

    simgrid::s4u::Storage* storage = simgrid::s4u::Storage::by_name("Disk4");

    // Now rename file from ./tmp/data.txt to ./tmp/simgrid.readme
    std::string newpath = "/home/tmp/simgrid.readme";
    XBT_INFO("Move '%s' to '%s'", file->get_path(), newpath.c_str());
    file->move(newpath);

    // Test attaching some user data to the file
    file->set_userdata(new std::string("777"));
    std::string* file_data = static_cast<std::string*>(file->get_userdata());
    XBT_INFO("User data attached to the file: %s", file_data->c_str());
    delete file_data;

    // Close the file
    delete file;

    // Now attach some user data to disk1
    XBT_INFO("Get/set data for storage element: %s", storage->get_cname());
    XBT_INFO("    Uninitialized storage data: '%s'", static_cast<char*>(storage->get_data()));

    storage->set_data(new std::string("Some user data"));
    std::string* storage_data = static_cast<std::string*>(storage->get_data());
    XBT_INFO("    Set and get data: '%s'", storage_data->c_str());

    delete storage_data;

    // Reopen the file and then unlink it
    file = new simgrid::s4u::File("/home/tmp/simgrid.readme", nullptr);
    XBT_INFO("Unlink file: '%s'", file->get_path());
    file->unlink();
    delete file; // Unlinking the file on "disk" does not free the object

    show_info(mounts);
  }
};

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("host", simgrid::s4u::Host::by_name("denise"), MyHost());
  e.run();

  return 0;
}
