/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <string>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

double start_time;
std::unordered_map<int, std::string> workernames;
std::unordered_map<int, std::string> masternames;

static void master(std::vector<std::string> args)
{
  xbt_assert(args.size() == 4, "Strange number of arguments expected 3 got %zu", args.size() - 1);

  XBT_DEBUG("Master started");

  /* data size */
  double msg_size = std::stod(args[1]);
  int id          = std::stoi(args[3]); // unique id to control statistics

  /* worker name */
  workernames[id] = args[2];

  sg4::Mailbox* mbox = sg4::Mailbox::by_name(args[3]);

  masternames[id] = sg4::Host::current()->get_name();

  auto* payload = new double(msg_size);

  /* time measurement */
  start_time = sg4::Engine::get_clock();
  mbox->put(payload, static_cast<uint64_t>(msg_size));

  XBT_DEBUG("Finished");
}

static void worker(std::vector<std::string> args)
{
  xbt_assert(args.size() == 2, "Strange number of arguments expected 1 got %zu", args.size() - 1);

  int id                      = std::stoi(args[1]);
  sg4::Mailbox* mbox          = sg4::Mailbox::by_name(args[1]);

  XBT_DEBUG("Worker started");

  auto payload = mbox->get_unique<double>();

  double elapsed_time = sg4::Engine::get_clock() - start_time;

  XBT_INFO("FLOW[%d] : Receive %.0f bytes from %s to %s", id, *payload, masternames.at(id).c_str(),
           workernames.at(id).c_str());
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

  e.register_function("master", master);
  e.register_function("worker", worker);

  e.load_deployment(argv[2]);

  e.run();

  return 0;
}
