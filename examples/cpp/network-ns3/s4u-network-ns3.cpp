/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

int timer_start; // set as 1 in the master actor

#define NTASKS 1500
double start_time;
const char* workernames[NTASKS];
const char* masternames[NTASKS];
int count_finished = 0;

static void master(int argc, char* argv[])
{
  xbt_assert(argc == 4, "Strange number of arguments expected 3 got %d", argc - 1);

  XBT_DEBUG("Master started");

  /* data size */
  double msg_size = std::stod(argv[1]);
  int id          = std::stoi(argv[3]); // unique id to control statistics

  /* worker name */
  workernames[id] = xbt_strdup(argv[2]);

  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name(argv[3]);

  masternames[id] = simgrid::s4u::Host::current()->get_cname();

  auto* payload = new double(msg_size);

  count_finished++;
  timer_start = 1;

  /* time measurement */
  start_time = simgrid::s4u::Engine::get_clock();
  mbox->put(payload, msg_size);

  XBT_DEBUG("Finished");
}

static void timer(int argc, char* argv[])
{
  xbt_assert(argc == 3, "Strange number of arguments expected 2 got %d", argc - 1);
  double first_sleep = std::stod(argv[1]);
  double sleep_time  = std::stod(argv[2]);

  XBT_DEBUG("Timer started");

  if (first_sleep)
    simgrid::s4u::this_actor::sleep_for(first_sleep);

  do {
    XBT_DEBUG("Get sleep");
    simgrid::s4u::this_actor::sleep_for(sleep_time);
  } while (timer_start);

  XBT_DEBUG("Finished");
}

static void worker(int argc, char* argv[])
{
  xbt_assert(argc == 2, "Strange number of arguments expected 1 got %d", argc - 1);

  int id                      = std::stoi(argv[1]);
  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name(argv[1]);

  XBT_DEBUG("Worker started");

  auto payload = mbox->get_unique<double>();

  count_finished--;
  if (count_finished == 0) {
    timer_start = 0;
  }

  double elapsed_time = simgrid::s4u::Engine::get_clock() - start_time;

  XBT_INFO("FLOW[%d] : Receive %.0f bytes from %s to %s", id, *payload, masternames[id], workernames[id]);
  XBT_DEBUG("FLOW[%d] : transferred in  %f seconds", id, elapsed_time);

  XBT_DEBUG("Finished");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s platform.xml deployment.xml\n",
             argv[0], argv[0]);

  e.load_platform(argv[1]);

  e.register_function("master", master);
  e.register_function("worker", worker);
  e.register_function("timer", timer);

  e.load_deployment(argv[2]);

  e.run();

  return 0;
}
