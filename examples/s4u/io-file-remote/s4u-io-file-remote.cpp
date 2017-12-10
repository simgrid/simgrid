/* Copyright (c) 2014-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
#include <src/plugins/file_system/FileSystem.hpp>
#include <string>

#define INMEGA (1024 * 1024)

XBT_LOG_NEW_DEFAULT_CATEGORY(remote_io, "Messages specific for this io example");

static int host(int argc, char* argv[])
{
  simgrid::s4u::File file(argv[1], nullptr);
  const char* filename = file.getPath();
  XBT_INFO("Opened file '%s'", filename);
  file.dump();

  XBT_INFO("Try to read %llu from '%s'", file.size(), filename);
  sg_size_t read = file.read(file.size());
  XBT_INFO("Have read %llu from '%s'. Offset is now at: %llu", read, filename, file.tell());
  XBT_INFO("Seek back to the beginning of the stream...");
  file.seek(0, SEEK_SET);
  XBT_INFO("Offset is now at: %llu", file.tell());

  if (argc > 5) {
    simgrid::s4u::File remoteFile(argv[2], nullptr);
    filename = remoteFile.getPath();
    XBT_INFO("Opened file '%s'", filename);
    XBT_INFO("Try to write %llu MiB to '%s'", remoteFile.size() / 1024, filename);
    sg_size_t write = remoteFile.write(remoteFile.size() * 1024);
    XBT_INFO("Have written %llu bytes to '%s'.", write, filename);

    if (std::stoi(argv[5]) != 0) {
      XBT_INFO("Move '%s' (of size %llu) from '%s' to '%s'", filename, remoteFile.size(),
               simgrid::s4u::Host::current()->getCname(), argv[3]);
      remoteFile.remoteMove(simgrid::s4u::Host::by_name(argv[3]), argv[4]);
    } else {
      XBT_INFO("Copy '%s' (of size %llu) from '%s' to '%s'", filename, remoteFile.size(),
               simgrid::s4u::Host::current()->getCname(), argv[3]);
      remoteFile.remoteCopy(simgrid::s4u::Host::by_name(argv[3]), argv[4]);
    }
  }

  return 0;
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.loadPlatform(argv[1]);
  e.registerFunction("host", host);
  e.loadDeployment(argv[2]);
  std::map<std::string, simgrid::s4u::Storage*> allStorages;
  simgrid::s4u::getStorageList(&allStorages);

  for (auto const& s : allStorages) {
    XBT_INFO("Init: %llu/%llu MiB used/free on '%s'", sg_storage_get_size_used(s.second) / INMEGA,
             sg_storage_get_size_free(s.second) / INMEGA, s.second->getCname());
  }

  e.run();

  for (auto const& s : allStorages) {
    XBT_INFO("End: %llu/%llu MiB used/free on '%s'", sg_storage_get_size_used(s.second) / INMEGA,
             sg_storage_get_size_free(s.second) / INMEGA, s.second->getCname());
  }

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::getClock());
  return 0;
}
