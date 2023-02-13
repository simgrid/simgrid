/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <string>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

struct MasterWorkerNames {
  std::string master;
  std::string worker;
};
using MasterWorkerNamesMap = std::unordered_map<int, MasterWorkerNames>;

struct Payload {
  double msg_size;
  double start_time;
};

static void master(MasterWorkerNamesMap& names, const std::vector<std::string>& args)
{
  xbt_assert(args.size() == 4, "Strange number of arguments expected 3 got %zu", args.size() - 1);

  XBT_DEBUG("Master started");

  /* data size */
  double msg_size = std::stod(args[1]);
  int id          = std::stoi(args[3]); // unique id to control statistics

  /* master and worker names */
  names.try_emplace(id, MasterWorkerNames{sg4::Host::current()->get_name(), args[2]});

  sg4::Mailbox* mbox = sg4::Mailbox::by_name(args[3]);

  auto* payload = new Payload{msg_size, sg4::Engine::get_clock()};
  mbox->put(payload, static_cast<uint64_t>(msg_size));

  XBT_DEBUG("Finished");
}

static void worker(const MasterWorkerNamesMap& names, const std::vector<std::string>& args)
{
  xbt_assert(args.size() == 2, "Strange number of arguments expected 1 got %zu", args.size() - 1);

  int id                      = std::stoi(args[1]);
  sg4::Mailbox* mbox          = sg4::Mailbox::by_name(args[1]);

  XBT_DEBUG("Worker started");

  auto payload = mbox->get_unique<Payload>();

  double elapsed_time = sg4::Engine::get_clock() - payload->start_time;

  XBT_INFO("FLOW[%d] : Receive %.0f bytes from %s to %s", id, payload->msg_size, names.at(id).master.c_str(),
           names.at(id).worker.c_str());
  XBT_DEBUG("FLOW[%d] : transferred in  %f seconds", id, elapsed_time);

  XBT_DEBUG("Finished");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s platform.xml deployment.xml\n",
             argv[0], argv[0]);

  e.load_platform(argv[1]);

  MasterWorkerNamesMap master_worker_names;
  e.register_function("master", [&master_worker_names](auto args) { master(master_worker_names, args); });
  e.register_function("worker", [&master_worker_names](auto args) { worker(master_worker_names, args); });

  e.load_deployment(argv[2]);

  e.run();

  return 0;
}
