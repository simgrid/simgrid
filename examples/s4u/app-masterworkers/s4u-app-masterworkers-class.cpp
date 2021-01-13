/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* ************************************************************************* */
/* Take this tutorial online: https://simgrid.frama.io/simgrid/tuto_s4u.html */
/* ************************************************************************* */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker, "Messages specific for this s4u example");

class Master {
  long tasks_count      = 0;
  double compute_cost   = 0;
  long communicate_cost = 0;
  std::vector<simgrid::s4u::Mailbox*> workers;

public:
  explicit Master(std::vector<std::string> args)
  {
    xbt_assert(args.size() > 4, "The master function expects 3 arguments plus the workers' names");

    tasks_count      = std::stol(args[1]);
    compute_cost     = std::stod(args[2]);
    communicate_cost = std::stol(args[3]);
    for (unsigned int i = 4; i < args.size(); i++)
      workers.push_back(simgrid::s4u::Mailbox::by_name(args[i]));

    XBT_INFO("Got %zu workers and %ld tasks to process", workers.size(), tasks_count);
  }

  void operator()()
  {
    for (int i = 0; i < tasks_count; i++) { /* For each task to be executed: */
      /* - Select a worker in a round-robin way */
      simgrid::s4u::Mailbox* mailbox = workers[i % workers.size()];

      /* - Send the computation amount to the worker */
      if (tasks_count < 10000 || (tasks_count < 100000 && i % 10000 == 0) || i % 100000 == 0)
        XBT_INFO("Sending task %d of %ld to mailbox '%s'", i, tasks_count, mailbox->get_cname());
      mailbox->put(new double(compute_cost), communicate_cost);
    }

    XBT_INFO("All tasks have been dispatched. Request all workers to stop.");
    for (unsigned int i = 0; i < workers.size(); i++) {
      /* The workers stop when receiving a negative compute_cost */
      simgrid::s4u::Mailbox* mailbox = workers[i % workers.size()];
      mailbox->put(new double(-1.0), 0);
    }
  }
};

class Worker {
  simgrid::s4u::Mailbox* mailbox = nullptr;

public:
  explicit Worker(std::vector<std::string> args)
  {
    xbt_assert(args.size() == 1, "The worker expects to not get any argument");

    mailbox = simgrid::s4u::Mailbox::by_name(simgrid::s4u::this_actor::get_host()->get_name());
  }

  void operator()()
  {
    double compute_cost;
    do {
      auto msg     = mailbox->get_unique<double>();
      compute_cost = *msg;

      if (compute_cost > 0) /* If compute_cost is valid, execute a computation of that cost */
        simgrid::s4u::this_actor::execute(compute_cost);
    } while (compute_cost > 0); /* Stop when receiving an invalid compute_cost */

    XBT_INFO("Exiting now.");
  }
};

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  /* Register the classes representing the actors */
  e.register_actor<Master>("master");
  e.register_actor<Worker>("worker");

  /* Load the platform description and then deploy the application */
  e.load_platform(argv[1]);
  e.load_deployment(argv[2]);

  /* Run the simulation */
  e.run();

  XBT_INFO("Simulation is over");

  return 0;
}
