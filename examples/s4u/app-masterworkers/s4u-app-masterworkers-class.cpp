/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker, "Messages specific for this s4u example");

class Master {
  long tasks_count                 = 0;
  double compute_cost              = 0;
  double communicate_cost          = 0;
  long workers_count               = 0;
  simgrid::s4u::MailboxPtr mailbox = nullptr;

public:
  explicit Master(std::vector<std::string> args)
  {
    xbt_assert(args.size() == 5, "The master actor expects 4 arguments from the XML deployment file");

    workers_count    = std::stol(args[1]);
    tasks_count      = std::stol(args[2]);
    compute_cost     = std::stod(args[3]);
    communicate_cost = std::stod(args[4]);

    XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, tasks_count);
  }

  void operator()()
  {
    for (int i = 0; i < tasks_count; i++) { /* For each task to be executed: */
      /* - Select a worker in a round-robin way */
      mailbox = simgrid::s4u::Mailbox::by_name(std::string("worker-") + std::to_string(i % workers_count));

      /* - Send the computation amount to the worker */
      if (tasks_count < 10000 || (tasks_count < 100000 && i % 10000 == 0) || i % 100000 == 0)
        XBT_INFO("Sending task %d of %ld to mailbox '%s'", i, tasks_count, mailbox->get_cname());
      mailbox->put(new double(compute_cost), communicate_cost);
    }

    XBT_INFO("All tasks have been dispatched. Request all workers to stop.");
    for (int i = 0; i < workers_count; i++) {
      /* The workers stop when receiving a negative compute_cost */
      mailbox = simgrid::s4u::Mailbox::by_name(std::string("worker-") + std::to_string(i));
      mailbox->put(new double(-1.0), 0);
    }
  }
};

class Worker {
  long id                          = -1;
  simgrid::s4u::MailboxPtr mailbox = nullptr;

public:
  explicit Worker(std::vector<std::string> args)
  {
    xbt_assert(args.size() == 2, "The worker expects a single argument from the XML deployment file: "
                                 "its worker ID (its numerical rank)");
    id      = std::stol(args[1]);
    mailbox = simgrid::s4u::Mailbox::by_name(std::string("worker-") + std::to_string(id));
  }

  void operator()()
  {
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
