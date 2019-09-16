/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/file_system.h>
#include <simgrid/s4u.hpp>
#include <xbt/string.hpp>

#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(storage, "Messages specific for this simulation");

static void display_storage_properties(simgrid::s4u::Storage* storage)
{
  const std::unordered_map<std::string, std::string>* props = storage->get_properties();
  if (not props->empty()) {
    XBT_INFO("  Properties of mounted storage: %s", storage->get_cname());

    for (auto const& elm : *props) {
      XBT_INFO("    %s->%s", elm.first.c_str(), elm.second.c_str());
    }
  } else {
    XBT_INFO("  No property attached.");
  }
}

static sg_size_t write_local_file(const std::string& dest, sg_size_t file_size)
{
  simgrid::s4u::File file(dest, nullptr);
  sg_size_t written = file.write(file_size);
  XBT_INFO("%llu bytes on %llu bytes have been written by %s on /sd1", written, file_size,
           simgrid::s4u::Actor::self()->get_cname());
  return written;
}

static sg_size_t read_local_file(const std::string& src)
{
  simgrid::s4u::File file(src, nullptr);
  sg_size_t file_size = file.size();
  sg_size_t read      = file.read(file_size);
  XBT_INFO("%s has read %llu on %s", simgrid::s4u::Actor::self()->get_cname(), read, src.c_str());
  return read;
}

// Read src file on local disk and send a put message to remote host (size of message = size of src file)
static void hsm_put(const std::string& remote_host, const std::string& src, const std::string& dest)
{
  // Read local src file, and return the size that was actually read
  sg_size_t read_size = read_local_file(src);

  // Send file
  XBT_INFO("%s sends %llu to %s", simgrid::s4u::this_actor::get_cname(), read_size, remote_host.c_str());
  std::string* payload             = new std::string(simgrid::xbt::string_printf("%s %llu", dest.c_str(), read_size));
  simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(remote_host);
  mailbox->put(payload, read_size);
  simgrid::s4u::this_actor::sleep_for(.4);
}

static void display_storage_content(simgrid::s4u::Storage* storage)
{
  XBT_INFO("Print the content of the storage element: %s", storage->get_cname());
  std::map<std::string, sg_size_t>* content = storage->extension<simgrid::s4u::FileSystemStorageExt>()->get_content();
  if (not content->empty()) {
    for (auto const& entry : *content)
      XBT_INFO("  %s size: %llu bytes", entry.first.c_str(), entry.second);
  } else {
    XBT_INFO("  No content.");
  }
}

static void dump_storage_by_name(const std::string& name)
{
  XBT_INFO("*** Dump a storage element ***");
  simgrid::s4u::Storage* storage = simgrid::s4u::Storage::by_name(name);
  display_storage_content(storage);
}

static void get_set_storage_data(const std::string& storage_name)
{
  XBT_INFO("*** GET/SET DATA for storage element: %s ***", storage_name.c_str());
  simgrid::s4u::Storage* storage = simgrid::s4u::Storage::by_name(storage_name);

  std::string* data = static_cast<std::string*>(storage->get_data());
  XBT_INFO("Get data: '%s'", data ? data->c_str() : "No User Data");
  storage->set_data(new std::string("Some data"));
  data = static_cast<std::string*>(storage->get_data());
  XBT_INFO("  Set and get data: '%s'", data->c_str());
  delete data;
}

static void dump_platform_storages()
{
  std::vector<simgrid::s4u::Storage*> storages = simgrid::s4u::Engine::get_instance()->get_all_storages();

  for (auto const& s : storages) {
    XBT_INFO("Storage %s is attached to %s", s->get_cname(), s->get_host()->get_cname());
    s->set_property("other usage", "gpfs");
  }
}

static void storage_info(simgrid::s4u::Host* host)
{
  XBT_INFO("*** Storage info on %s ***", host->get_cname());

  for (auto const& elm : host->get_mounted_storages()) {
    const std::string& mount_name  = elm.first;
    simgrid::s4u::Storage* storage = elm.second;
    XBT_INFO("  Storage name: %s, mount name: %s", storage->get_cname(), mount_name.c_str());

    XBT_INFO("    Free size: %llu bytes", sg_storage_get_size_free(storage));
    XBT_INFO("    Used size: %llu bytes", sg_storage_get_size_used(storage));

    display_storage_properties(storage);
    dump_storage_by_name(storage->get_cname());
  }
}

static void client()
{
  hsm_put("alice", "/home/doc/simgrid/examples/msg/icomms/small_platform.xml", "/tmp/toto.xml");
  hsm_put("alice", "/home/doc/simgrid/examples/msg/parallel_task/test_ptask_deployment.xml", "/tmp/titi.xml");
  hsm_put("alice", "/home/doc/simgrid/examples/msg/alias/masterslave_forwarder_with_alias.c", "/tmp/tata.c");

  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("alice");
  mailbox->put(new std::string("finalize"), 0);

  get_set_storage_data("Disk1");
}

static void server()
{
  storage_info(simgrid::s4u::this_actor::get_host());
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(simgrid::s4u::this_actor::get_host()->get_cname());

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

  storage_info(simgrid::s4u::this_actor::get_host());
  dump_platform_storages();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();
  xbt_assert(argc == 2, "Usage: %s platform_file\n", argv[0]);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("server", simgrid::s4u::Host::by_name("alice"), server);
  simgrid::s4u::Actor::create("client", simgrid::s4u::Host::by_name("bob"), client);

  e.run();

  XBT_INFO("Simulated time: %g", e.get_clock());
  return 0;
}
