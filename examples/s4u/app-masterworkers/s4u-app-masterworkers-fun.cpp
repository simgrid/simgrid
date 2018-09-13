/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* ************************************************************************* */
/* Take this tutorial online: https://simgrid.frama.io/simgrid/tuto_s4u.html */
/* ************************************************************************* */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker, "Messages specific for this example");

// master-begin
static void master(std::vector<std::string> args)
{
  xbt_assert(args.size() > 4, "The master function expects at least 3 arguments");

  long tasks_count          = std::stol(args[1]);
  double compute_cost       = std::stod(args[2]);
  double communication_cost = std::stod(args[3]);
  std::vector<simgrid::s4u::MailboxPtr> workers;
  for (unsigned int i = 4; i < args.size(); i++)
    workers.push_back(simgrid::s4u::Mailbox::by_name(args[i]));

  XBT_INFO("Got %zu workers and %ld tasks to process", workers.size(), tasks_count);

  for (int i = 0; i < tasks_count; i++) { /* For each task to be executed: */
    /* - Select a worker in a round-robin way */
    simgrid::s4u::MailboxPtr mailbox = workers[i % workers.size()];

    /* - Send the computation cost to that worker */
    XBT_INFO("Sending task %d of %ld to mailbox '%s'", i, tasks_count, mailbox->get_cname());
    mailbox->put(new double(compute_cost), communication_cost);
  }

  XBT_INFO("All tasks have been dispatched. Request all workers to stop.");
  for (unsigned int i = 0; i < workers.size(); i++) {
    /* The workers stop when receiving a negative compute_cost */
    simgrid::s4u::MailboxPtr mailbox = workers[i % workers.size()];

    mailbox->put(new double(-1.0), 0);
  }
}
// master-end

// worker-begin
static void worker(std::vector<std::string> args)
{
  xbt_assert(args.size() == 1, "The worker expects no argument");

  simgrid::s4u::Host* my_host      = simgrid::s4u::this_actor::get_host();
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::by_name(my_host->get_name());

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
// worker-end

// main-begin
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
// main-end
