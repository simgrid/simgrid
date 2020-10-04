/* Copyright (c) 2013-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/file_system.h>
#include <simgrid/s4u.hpp>
#include <xbt/string.hpp>

#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(storage, "Messages specific for this simulation");

static void display_disk_properties(const simgrid::s4u::Disk* disk)
{
  const std::unordered_map<std::string, std::string>* props = disk->get_properties();
  if (not props->empty()) {
    XBT_INFO("  Properties of disk: %s", disk->get_cname());

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
  auto* payload = new std::string(simgrid::xbt::string_printf("%s %llu", dest.c_str(), read_size));
  auto* mailbox = simgrid::s4u::Mailbox::by_name(remote_host);
  mailbox->put(payload, read_size);
  simgrid::s4u::this_actor::sleep_for(.4);
}

static void display_disk_content(const simgrid::s4u::Disk* disk)
{
  XBT_INFO("*** Dump a disk ***");
  XBT_INFO("Print the content of the disk: %s", disk->get_cname());
  const std::map<std::string, sg_size_t>* content = disk->extension<simgrid::s4u::FileSystemDiskExt>()->get_content();
  if (not content->empty()) {
    for (auto const& entry : *content)
      XBT_INFO("  %s size: %llu bytes", entry.first.c_str(), entry.second);
  } else {
    XBT_INFO("  No content.");
  }
}

static void get_set_disk_data(simgrid::s4u::Disk* disk)
{
  XBT_INFO("*** GET/SET DATA for disk: %s ***", disk->get_cname());

  const std::string* data = static_cast<std::string*>(disk->get_data());
  XBT_INFO("Get data: '%s'", data ? data->c_str() : "No User Data");
  disk->set_data(new std::string("Some data"));
  data = static_cast<std::string*>(disk->get_data());
  XBT_INFO("  Set and get data: '%s'", data->c_str());
  delete data;
}

static void dump_platform_disks()
{
  for (auto const& h : simgrid::s4u::Engine::get_instance()->get_all_hosts())
    for (auto const& d : h->get_disks()) {
      if (h == d->get_host())
        XBT_INFO("%s is attached to %s", d->get_cname(), d->get_host()->get_cname());
      d->set_property("other usage", "gpfs");
    }
}

static void disk_info(const simgrid::s4u::Host* host)
{
  XBT_INFO("*** Disk info on %s ***", host->get_cname());

  for (auto const& disk : host->get_disks()) {
    const char* mount_name = sg_disk_get_mount_point(disk);
    XBT_INFO("  Disk name: %s, mount name: %s", disk->get_cname(), mount_name);

    XBT_INFO("    Free size: %llu bytes", sg_disk_get_size_free(disk));
    XBT_INFO("    Used size: %llu bytes", sg_disk_get_size_used(disk));

    display_disk_properties(disk);
    display_disk_content(disk);
  }
}

static void client()
{
  hsm_put("alice", "/scratch/doc/simgrid/examples/msg/icomms/small_platform.xml", "/tmp/toto.xml");
  hsm_put("alice", "/scratch/doc/simgrid/examples/msg/parallel_task/test_ptask_deployment.xml", "/tmp/titi.xml");
  hsm_put("alice", "/scratch/doc/simgrid/examples/msg/alias/masterslave_forwarder_with_alias.c", "/tmp/tata.c");

  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("alice");
  mailbox->put(new std::string("finalize"), 0);

  get_set_disk_data(simgrid::s4u::Host::current()->get_disks().front()); // Disk1
}

static void server()
{
  disk_info(simgrid::s4u::this_actor::get_host());
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(simgrid::s4u::this_actor::get_host()->get_cname());

  XBT_INFO("Server waiting for transfers ...");
  while (true) {
    const std::string* msg = static_cast<std::string*>(mailbox->get());
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

  disk_info(simgrid::s4u::this_actor::get_host());
  dump_platform_disks();
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
