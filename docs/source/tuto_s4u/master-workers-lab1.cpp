/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/****************************************************************************
 *
 *     This is our solution to the first lab of the S4U tutorial
 * (available online at https://simgrid.frama.io/simgrid/tuto_s4u.html)
 *
 *    Reading this further before taking the tutorial will SPOIL YOU!!!
 *
 ****************************************************************************/

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker, "Messages specific for this example");

static void master(std::vector<std::string> args)
{
  xbt_assert(args.size() == 5, "The master function expects 4 arguments");

  long workers_count        = std::stol(args[1]);
  long tasks_count          = std::stol(args[2]);
  double compute_cost       = std::stod(args[3]);
  double communication_cost = std::stod(args[4]);

  XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, tasks_count);

  for (int i = 0; i < tasks_count; i++) { /* For each task to be executed: */
    /* - Select a worker in a round-robin way */
    std::string worker_rank          = std::to_string(i % workers_count);
    std::string mailbox_name         = std::string("worker-") + worker_rank;
    simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(mailbox_name);

    /* - Send the computation cost to that worker */
    XBT_INFO("Sending task %d of %ld to mailbox '%s'", i, tasks_count, mailbox->get_cname());
    mailbox->put(new double(compute_cost), communication_cost);
  }

  XBT_INFO("All tasks have been dispatched. Request all workers to stop.");
  for (int i = 0; i < workers_count; i++) {
    /* The workers stop when receiving a negative compute_cost */
    std::string mailbox_name         = std::string("worker-") + std::to_string(i);
    simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(mailbox_name);

    mailbox->put(new double(-1.0), 0);
  }
}

static void worker(std::vector<std::string> args)
{
  xbt_assert(args.size() == 2, "The worker expects a single argument");
  long id = std::stol(args[1]);

  const std::string mailbox_name   = std::string("worker-") + std::to_string(id);
  simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(mailbox_name);

  double compute_cost;
  do {
    double* msg  = static_cast<double*>(mailbox->get());
    compute_cost = *msg;
    delete msg;

    if (compute_cost > 0) /* If compute_cost is valid, execute a computation of that cost */
      simgrid::s4u::this_actor::execute(compute_cost);
  } while (compute_cost > 0); /* Stop when receiving an invalid compute_cost */

  XBT_INFO("Exiting now.");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  /* Register the functions representing the actors */
  e.register_function("master", &master);
  e.register_function("worker", &worker);

  /* Load the platform description and then deploy the application */
  e.load_platform(argv[1]);
  e.load_deployment(argv[2]);

  /* Run the simulation */
  e.run();

  XBT_INFO("Simulation is over");

  return 0;
}
