/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(storage, "Messages specific for this simulation");

static void display_storage_properties(simgrid::s4u::Storage* storage)
{
  std::unordered_map<std::string, std::string>* props = storage->getProperties();
  if (not props->empty()) {
    XBT_INFO("\tProperties of mounted storage: %s", storage->getName());

    for (auto elm : *props) {
      XBT_INFO("    %s->%s", elm.first.c_str(), elm.second.c_str());
    }
  } else {
    XBT_INFO("\tNo property attached.");
  }
}

static sg_size_t write_local_file(const char* dest, sg_size_t file_size)
{
  simgrid::s4u::File* file = new simgrid::s4u::File(dest, nullptr);
  sg_size_t written        = file->write(file_size);
  XBT_INFO("%llu bytes on %llu bytes have been written by %s on /sd1", written, file_size,
           simgrid::s4u::Actor::self()->getName().c_str());
  delete file;
  return written;
}

static sg_size_t read_local_file(const char* src)
{
  simgrid::s4u::File* file = new simgrid::s4u::File(src, nullptr);
  sg_size_t file_size      = file->size();
  sg_size_t read           = file->read(file_size);

  XBT_INFO("%s has read %llu on %s", simgrid::s4u::Actor::self()->getName().c_str(), read, src);
  delete file;

  return read;
}

// Read src file on local disk and send a put message to remote host (size of message = size of src file)
static void hsm_put(const char* remote_host, const char* src, const char* dest)
{
  // Read local src file, and return the size that was actually read
  sg_size_t read_size = read_local_file(src);

  // Send file
  XBT_INFO("%s sends %llu to %s", simgrid::s4u::this_actor::getName().c_str(), read_size, remote_host);
  char* payload                    = bprintf("%s %llu", dest, read_size);
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(remote_host);
  mailbox->put(payload, static_cast<double>(read_size));
  simgrid::s4u::this_actor::sleep_for(.4);
}

static void display_storage_content(simgrid::s4u::Storage* storage)
{
  XBT_INFO("Print the content of the storage element: %s", storage->getName());
  std::map<std::string, sg_size_t>* content = storage->getContent();
  if (not content->empty()) {
    for (auto entry : *content)
      XBT_INFO("\t%s size: %llu bytes", entry.first.c_str(), entry.second);
  } else {
    XBT_INFO("\tNo content.");
  }
}

static void dump_storage_by_name(const char* name)
{
  XBT_INFO("*** Dump a storage element ***");
  simgrid::s4u::Storage* storage = simgrid::s4u::Storage::byName(name);
  display_storage_content(storage);
}

static void get_set_storage_data(const char* storage_name)
{
  XBT_INFO("*** GET/SET DATA for storage element: %s ***", storage_name);
  simgrid::s4u::Storage* storage = simgrid::s4u::Storage::byName(storage_name);

  char* data = static_cast<char*>(storage->getUserdata());
  XBT_INFO("Get data: '%s'", data);
  storage->setUserdata(xbt_strdup("Some data"));
  data = static_cast<char*>(storage->getUserdata());
  XBT_INFO("\tSet and get data: '%s'", data);
  xbt_free(data);
}

static void dump_platform_storages()
{
  std::map<std::string, simgrid::s4u::Storage*>* storages = simgrid::s4u::allStorages();

  for (auto storage : *storages) {
    XBT_INFO("Storage %s is attached to %s", storage.first.c_str(), storage.second->getHost()->getCname());
    storage.second->setProperty("other usage", "gpfs");
  }
  delete storages;
}

static void storage_info(simgrid::s4u::Host* host)
{
  XBT_INFO("*** Storage info on %s ***", host->getCname());

  for (auto elm : host->getMountedStorages()) {
    const char* mount_name         = elm.first.c_str();
    simgrid::s4u::Storage* storage = elm.second;
    XBT_INFO("\tStorage name: %s, mount name: %s", storage->getName(), mount_name);

    sg_size_t free_size = storage->getSizeFree();
    sg_size_t used_size = storage->getSizeUsed();

    XBT_INFO("\t\tFree size: %llu bytes", free_size);
    XBT_INFO("\t\tUsed size: %llu bytes", used_size);

    display_storage_properties(storage);
    dump_storage_by_name(storage->getName());
  }
}

static void client()
{
  hsm_put("alice", "/home/doc/simgrid/examples/msg/icomms/small_platform.xml", "c:\\Windows\\toto.cxx");
  hsm_put("alice", "/home/doc/simgrid/examples/msg/parallel_task/test_ptask_deployment.xml", "c:\\Windows\\titi.xml");
  hsm_put("alice", "/home/doc/simgrid/examples/msg/alias/masterslave_forwarder_with_alias.c", "c:\\Windows\\tata.c");

  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName("alice");
  mailbox->put(xbt_strdup("finalize"), 0);

  get_set_storage_data("Disk1");
}

static void server()
{
  storage_info(simgrid::s4u::this_actor::getHost());
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(simgrid::s4u::this_actor::getHost()->getCname());

  XBT_INFO("Server waiting for transfers ...");
  while (1) {
    char* msg = static_cast<char*>(mailbox->get());
    if (not strcmp(msg, "finalize")) { // Shutdown ...
      xbt_free(msg);
      break;
    } else { // Receive file to save
      char* saveptr;
      char* dest              = strtok_r(msg, " ", &saveptr);
      sg_size_t size_to_write = std::stoull(strtok_r(nullptr, " ", &saveptr));
      write_local_file(dest, size_to_write);
      xbt_free(dest);
    }
  }

  storage_info(simgrid::s4u::this_actor::getHost());
  dump_platform_storages();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine* e = new simgrid::s4u::Engine(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n", argv[0]);
  e->loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("server", simgrid::s4u::Host::by_name("alice"), server);
  simgrid::s4u::Actor::createActor("client", simgrid::s4u::Host::by_name("bob"), client);

  e->run();

  XBT_INFO("Simulated time: %g", e->getClock());

  delete e;
  return 0;
}
