/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* ************************************************************************* */
/* Take this tutorial online: https://simgrid.org/doc/latest/Tutorial_Algorithms.html */
/* ************************************************************************* */

import java.util.ArrayList;
import java.util.List;
import org.simgrid.s4u.*;

// master-begin
class Master extends Actor {
  long tasks_count;
  double compute_cost;
  long communicate_cost;
  List<Mailbox> workers = new ArrayList<>();

  public Master(String[] args)
  {
    if (args.length < 4)
      Engine.die("The master function expects 3 arguments plus the workers' names");

    tasks_count      = Long.parseLong(args[0]);
    compute_cost     = Double.parseDouble(args[1]);
    communicate_cost = Long.parseLong(args[2]);
    for (int i = 3; i < args.length; i++)
      workers.add(this.get_engine().mailbox_by_name(args[i]));

    Engine.info("Got %d workers and %d tasks to process", workers.size(), tasks_count);
  }

  public void run() throws SimgridException
  {
    for (int i = 0; i < tasks_count; i++) { /* For each task to be executed: */
      /* - Select a worker in a round-robin way */
      Mailbox mailbox = workers.get(i % workers.size());

      /* - Send the computation amount to the worker */
      if (tasks_count < 10000 || (tasks_count < 100000 && i % 10000 == 0) || i % 100000 == 0)
        Engine.info("Sending task %d of %d to mailbox '%s'", i, tasks_count, mailbox.get_name());
      mailbox.put(compute_cost, communicate_cost);
    }

    Engine.info("All tasks have been dispatched. Request all workers to stop.");
    for (int i = 0; i < workers.size(); i++) {
      /* The workers stop when receiving a negative compute_cost */
      Mailbox mailbox = workers.get(i % workers.size());
      mailbox.put(-1.0, 0);
    }
  }
}
// master-end

// worker-begin
class Worker extends Actor {
  Mailbox mailbox;

  public Worker(String[] args)
  {
    if (args.length != 0)
      Engine.die("The worker expects to not get any argument");
  }

  public void run() throws SimgridException
  {
    mailbox = this.get_engine().mailbox_by_name(this.get_host().get_name());

    double compute_cost;
    do {
      compute_cost = (Double)mailbox.get();

      if (compute_cost > 0) /* If compute_cost is valid, execute a computation of that cost */
        execute(compute_cost);
    } while (compute_cost > 0); /* Stop when receiving an invalid compute_cost */

    Engine.info("Exiting now.");
  }
}
// worker-end

public class app_masterworkers {
  // main-begin
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    if (args.length < 2)
      Engine.die("Usage: app_masterworkers platform_file deployment_file\n");

    /* Load the platform description and then deploy the application */
    e.load_platform(args[0]);
    e.load_deployment(args[1]);

    /* Run the simulation */
    e.run();

    Engine.info("Simulation is over");
  }
  // main-end
}
