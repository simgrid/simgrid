/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/****************************************************************************
 *
 *     This is our solution to the second lab of the S4U tutorial
 * (available online at https://simgrid.frama.io/simgrid/tuto_s4u.html)
 *
 *    Reading this further before taking the tutorial will SPOIL YOU!!!
 *
 ****************************************************************************/

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker, "Messages specific for this example");

static void worker()
{
  const std::string mailbox_name   = std::string("worker-") + std::to_string(simgrid::s4u::this_actor::get_pid());
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

static void master(std::vector<std::string> args)
{
  xbt_assert(args.size() == 4, "The master function expects 3 arguments");

  long tasks_count          = std::stol(args[1]);
  double compute_cost       = std::stod(args[2]);
  double communication_cost = std::stod(args[3]);

  std::vector<simgrid::s4u::ActorPtr> actors;

  for (auto* host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    simgrid::s4u::ActorPtr act = simgrid::s4u::Actor::create(std::string("Worker-") + host->get_name(), host, worker);
    actors.push_back(act);
  }

  XBT_INFO("Got %ld tasks to process", tasks_count);

  for (int i = 0; i < tasks_count; i++) { /* For each task to be executed: */
    /* - Select a worker in a round-robin way */
    aid_t worker_pid                 = actors.at(i % actors.size())->get_pid();
    std::string mailbox_name         = std::string("worker-") + std::to_string(worker_pid);
    simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(mailbox_name);

    /* - Send the computation cost to that worker */
    XBT_INFO("Sending task %d of %ld to mailbox '%s'", i, tasks_count, mailbox->get_cname());
    mailbox->put(new double(compute_cost), communication_cost);
  }

  XBT_INFO("All tasks have been dispatched. Request all workers to stop.");
  for (unsigned long i = 0; i < actors.size(); i++) {
    /* The workers stop when receiving a negative compute_cost */
    std::string mailbox_name         = std::string("worker-") + std::to_string(actors.at(i)->get_pid());
    simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(mailbox_name);

    mailbox->put(new double(-1.0), 0);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  /* Register the functions representing the actors */
  e.register_function("master", &master);

  /* Load the platform description and then deploy the application */
  e.load_platform(argv[1]);
  e.load_deployment(argv[2]);

  /* Run the simulation */
  e.run();

  XBT_INFO("Simulation is over");

  return 0;
}
