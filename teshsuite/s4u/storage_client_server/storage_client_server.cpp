/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "src/plugins/file_system/FileSystem.hpp"
#include <string>
#include <xbt/string.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(storage, "Messages specific for this simulation");

static void display_storage_properties(simgrid::s4u::Storage* storage)
{
  std::map<std::string, std::string>* props = storage->getProperties();
  if (not props->empty()) {
    XBT_INFO("\tProperties of mounted storage: %s", storage->getCname());

    for (auto const& elm : *props) {
      XBT_INFO("    %s->%s", elm.first.c_str(), elm.second.c_str());
    }
  } else {
    XBT_INFO("\tNo property attached.");
  }
}

static sg_size_t write_local_file(const std::string& dest, sg_size_t file_size)
{
  simgrid::s4u::File file(dest, nullptr);
  sg_size_t written = file.write(file_size);
  XBT_INFO("%llu bytes on %llu bytes have been written by %s on /sd1", written, file_size,
           simgrid::s4u::Actor::self()->getCname());
  return written;
}

static sg_size_t read_local_file(const std::string& src)
{
  simgrid::s4u::File file(src, nullptr);
  sg_size_t file_size = file.size();
  sg_size_t read      = file.read(file_size);
  XBT_INFO("%s has read %llu on %s", simgrid::s4u::Actor::self()->getCname(), read, src.c_str());
  return read;
}

// Read src file on local disk and send a put message to remote host (size of message = size of src file)
static void hsm_put(const std::string& remote_host, const std::string& src, const std::string& dest)
{
  // Read local src file, and return the size that was actually read
  sg_size_t read_size = read_local_file(src);

  // Send file
  XBT_INFO("%s sends %llu to %s", simgrid::s4u::this_actor::getCname(), read_size, remote_host.c_str());
  std::string* payload             = new std::string(simgrid::xbt::string_printf("%s %llu", dest.c_str(), read_size));
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(remote_host);
  mailbox->put(payload, static_cast<double>(read_size));
  simgrid::s4u::this_actor::sleep_for(.4);
}

static void display_storage_content(simgrid::s4u::Storage* storage)
{
  XBT_INFO("Print the content of the storage element: %s", storage->getCname());
  std::map<std::string, sg_size_t>* content = storage->extension<simgrid::s4u::FileSystemStorageExt>()->getContent();
  if (not content->empty()) {
    for (auto const& entry : *content)
      XBT_INFO("\t%s size: %llu bytes", entry.first.c_str(), entry.second);
  } else {
    XBT_INFO("\tNo content.");
  }
}

static void dump_storage_by_name(const std::string& name)
{
  XBT_INFO("*** Dump a storage element ***");
  simgrid::s4u::Storage* storage = simgrid::s4u::Storage::byName(name);
  display_storage_content(storage);
}

static void get_set_storage_data(const std::string& storage_name)
{
  XBT_INFO("*** GET/SET DATA for storage element: %s ***", storage_name.c_str());
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
  std::map<std::string, simgrid::s4u::Storage*>* storages = new std::map<std::string, simgrid::s4u::Storage*>;
  simgrid::s4u::getStorageList(storages);

  for (auto const& storage : *storages) {
    XBT_INFO("Storage %s is attached to %s", storage.first.c_str(), storage.second->getHost()->getCname());
    storage.second->setProperty("other usage", "gpfs");
  }
  delete storages;
}

static void storage_info(simgrid::s4u::Host* host)
{
  XBT_INFO("*** Storage info on %s ***", host->getCname());

  for (auto const& elm : host->getMountedStorages()) {
    const std::string& mount_name  = elm.first;
    simgrid::s4u::Storage* storage = elm.second;
    XBT_INFO("\tStorage name: %s, mount name: %s", storage->getCname(), mount_name.c_str());

    XBT_INFO("\t\tFree size: %llu bytes", sg_storage_get_size_free(storage));
    XBT_INFO("\t\tUsed size: %llu bytes", sg_storage_get_size_used(storage));

    display_storage_properties(storage);
    dump_storage_by_name(storage->getCname());
  }
}

static void client()
{
  hsm_put("alice", "/home/doc/simgrid/examples/msg/icomms/small_platform.xml", "c:\\Windows\\toto.cxx");
  hsm_put("alice", "/home/doc/simgrid/examples/msg/parallel_task/test_ptask_deployment.xml", "c:\\Windows\\titi.xml");
  hsm_put("alice", "/home/doc/simgrid/examples/msg/alias/masterslave_forwarder_with_alias.c", "c:\\Windows\\tata.c");

  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName("alice");
  mailbox->put(new std::string("finalize"), 0);

  get_set_storage_data("Disk1");
}

static void server()
{
  storage_info(simgrid::s4u::this_actor::getHost());
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(simgrid::s4u::this_actor::getHost()->getCname());

  XBT_INFO("Server waiting for transfers ...");
  while (1) {
    std::string* msg = static_cast<std::string*>(mailbox->get());
    if (*msg == "finalize") { // Shutdown ...
      delete msg;
      break;
    } else { // Receive file to save
      size_t pos              = msg->find(' ');
      std::string dest        = msg->substr(0, pos);
      sg_size_t size_to_write = std::stoull(msg->substr(pos + 1));
      write_local_file(dest, size_to_write);
      delete msg;
    }
  }

  storage_info(simgrid::s4u::this_actor::getHost());
  dump_platform_storages();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();
  xbt_assert(argc == 2, "Usage: %s platform_file\n", argv[0]);
  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("server", simgrid::s4u::Host::by_name("alice"), server);
  simgrid::s4u::Actor::createActor("client", simgrid::s4u::Host::by_name("bob"), client);

  e.run();

  XBT_INFO("Simulated time: %g", e.getClock());
  return 0;
}
