/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/****************************************************************************
 *
 *     This is our solution to the fourth lab of the S4U tutorial
 * (available online at https://simgrid.frama.io/simgrid/tuto_s4u.html)
 *
 *    Reading this further before taking the tutorial will SPOIL YOU!!!
 *
 ****************************************************************************/

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker, "Messages specific for this example");

static void worker(std::string category)
{
  const std::string mailbox_name   = std::string("worker-") + std::to_string(simgrid::s4u::this_actor::get_pid());
  simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(mailbox_name);

  while (true) { // Master forcefully kills the workers by the end of the simulation
    double* msg         = static_cast<double*>(mailbox->get());
    double compute_cost = *msg;
    delete msg;

    // simgrid::s4u::this_actor::exec_init(compute_cost)->set_tracing_category(category)->wait();
    /* Long form:*/
    simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(compute_cost);
    exec->set_tracing_category(category);
    exec->wait();
  }
}

static void master(std::vector<std::string> args)
{
  xbt_assert(args.size() == 4, "The master function expects 3 arguments");

  double simulation_duration = std::stod(args[1]);
  double compute_cost        = std::stod(args[2]);
  double communication_cost  = std::stod(args[3]);

  std::vector<simgrid::s4u::ActorPtr> actors;

  simgrid::s4u::Engine* e = simgrid::s4u::Engine::get_instance();
  std::string my_name     = std::string("master-") + std::to_string(simgrid::s4u::this_actor::get_pid());

  XBT_INFO("Asked to run for %.1f seconds", simulation_duration);

  for (auto* host : e->get_all_hosts()) {
    simgrid::s4u::ActorPtr act =
        simgrid::s4u::Actor::create(std::string("Worker-") + host->get_name(), host, worker, my_name);
    actors.push_back(act);
  }

  int task_id = 0;
  while (e->get_clock() < simulation_duration) { /* For each task: */
    /* - Select a worker in a round-robin way */
    aid_t worker_pid                 = actors.at(task_id % actors.size())->get_pid();
    std::string mailbox_name         = std::string("worker-") + std::to_string(worker_pid);
    simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(mailbox_name);

    /* - Send the computation cost to that worker */
    XBT_DEBUG("Sending task %d to mailbox '%s'", task_id, mailbox->get_cname());
    mailbox->put(new double(compute_cost), communication_cost);

    task_id++;
  }

  XBT_INFO("Time is up. Forcefully kill all workers.");
  for (unsigned long i = 0; i < actors.size(); i++) {
    actors.at(i)->kill();
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
