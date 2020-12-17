/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mpi.h"
#include "simgrid/s4u.hpp"

#include <array>
#include <cstdio> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static void master(std::vector<std::string> args)
{
  xbt_assert(args.size() > 4, "The master function expects at least 3 arguments");

  long tasks_count        = std::stol(args[1]);
  double compute_cost     = std::stod(args[2]);
  long communication_cost = std::stol(args[3]);
  std::vector<simgrid::s4u::Mailbox*> workers;
  for (unsigned int i = 4; i < args.size(); i++)
    workers.push_back(simgrid::s4u::Mailbox::by_name(args[i]));

  XBT_INFO("Got %zu workers and %ld tasks to process", workers.size(), tasks_count);

  for (int i = 0; i < tasks_count; i++) { /* For each task to be executed: */
    /* - Select a worker in a round-robin way */
    simgrid::s4u::Mailbox* mailbox = workers[i % workers.size()];

    /* - Send the computation cost to that worker */
    XBT_INFO("Sending task %d of %ld to mailbox '%s'", i, tasks_count, mailbox->get_cname());
    mailbox->put(new double(compute_cost), communication_cost);
  }

  XBT_INFO("All tasks have been dispatched. Request all workers to stop.");
  for (unsigned int i = 0; i < workers.size(); i++) {
    /* The workers stop when receiving a negative compute_cost */
    simgrid::s4u::Mailbox* mailbox = workers[i % workers.size()];

    mailbox->put(new double(-1.0), 0);
  }
}

static void worker(std::vector<std::string> args)
{
  xbt_assert(args.size() == 1, "The worker expects no argument");

  const simgrid::s4u::Host* my_host = simgrid::s4u::this_actor::get_host();
  simgrid::s4u::Mailbox* mailbox    = simgrid::s4u::Mailbox::by_name(my_host->get_name());

  double compute_cost;
  do {
    auto msg     = mailbox->get_unique<double>();
    compute_cost = *msg;

    if (compute_cost > 0) /* If compute_cost is valid, execute a computation of that cost */
      simgrid::s4u::this_actor::execute(compute_cost);
  } while (compute_cost > 0); /* Stop when receiving an invalid compute_cost */

  XBT_INFO("Exiting now.");
}

static void master_mpi(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  XBT_INFO("here for rank %d", rank);
  std::array<int, 1000> test{{rank}};
  if (rank == 0)
    MPI_Send(test.data(), 1000, MPI_INT, 1, 1, MPI_COMM_WORLD);
  else
    MPI_Recv(test.data(), 1000, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

  XBT_INFO("After comm %d", rank);
  MPI_Finalize();

  XBT_INFO("After finalize %d %d", rank, test[0]);
}

static void alltoall_mpi(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  int rank;
  int size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  XBT_INFO("alltoall for rank %d", rank);
  std::vector<int> out(1000 * size);
  std::vector<int> in(1000 * size);
  MPI_Alltoall(out.data(), 1000, MPI_INT, in.data(), 1000, MPI_INT, MPI_COMM_WORLD);

  XBT_INFO("after alltoall %d", rank);
  MPI_Finalize();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  SMPI_init();

  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\nexample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  e.load_platform(argv[1]);

  e.register_function("master", master);
  e.register_function("worker", worker);
  // launch two MPI applications as well, one using master_mpi function as main on 2 nodes
  SMPI_app_instance_register("master_mpi", master_mpi, 2);
  // the second performing an alltoall on 4 nodes
  SMPI_app_instance_register("alltoall_mpi", alltoall_mpi, 4);
  e.load_deployment(argv[2]);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  SMPI_finalize();
  return 0;
}
