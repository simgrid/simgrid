/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Parallel activities are convenient abstractions of parallel computational kernels that span over several machines.
 * To create a new one, you have to provide several things:
 *   - a vector of hosts on which the activity will execute
 *   - a vector of values, the amount of computation for each of the hosts (in flops)
 *   - a matrix of values, the amount of communication between each pair of hosts (in bytes)
 *
 * Each of these operation will be processed at the same relative speed.
 * This means that at some point in time, all sub-executions and all sub-communications will be at 20% of completion.
 * Also, they will all complete at the exact same time.
 *
 * This is obviously a simplistic abstraction, but this is very handful in a large amount of situations.
 *
 * Please note that you must have the LV07 platform model enabled to use such constructs.
 */

import org.simgrid.s4u.*;

class runner extends Actor {
  public void run() throws TimeoutException, HostFailureException
  {
    /* Retrieve the list of all hosts as an array of hosts */
    var hosts                      = this.get_engine().get_all_hosts();
    int hosts_count                = hosts.length;
    double[] computation_amounts   = new double[hosts_count];
    double[] communication_amounts = new double[hosts_count * hosts_count];

    /* ------[ test 1 ]----------------- */
    Engine.info("First, build a classical parallel activity, with 1 Gflop to execute on each node, "
                + "and 10MB to exchange between each pair");

    for (int i = 0; i < hosts_count; i++) {
      computation_amounts[i] = 1e9 /*1Gflop*/;
      for (int j = i + 1; j < hosts_count; j++)
        communication_amounts[i * hosts_count + j] = 1e7; // 10 MB
    }

    this.parallel_execute(hosts, computation_amounts, communication_amounts);

    /* ------[ test 2 ]----------------- */
    Engine.info("We can do the same with a timeout of 10 seconds enabled.");
    for (int i = 0; i < hosts_count; i++) {
      computation_amounts[i] = 1e9 /*1Gflop*/;
      for (int j = i + 1; j < hosts_count; j++)
        communication_amounts[i * hosts_count + j] = 1e7; // 10 MB
    }

    Exec activity = this.exec_init(hosts, computation_amounts, communication_amounts);
    try {
      activity.await_for(10.0 /* timeout (in seconds)*/);
      Engine.die("Woops, this did not timeout as expected... Please report that bug.");
    } catch (TimeoutException ex) {
      Engine.info("Caught the expected timeout exception.");
      activity.cancel();
    }

    /* ------[ test 3 ]----------------- */
    Engine.info(
        "Then, build a parallel activity involving only computations (of different amounts) and no communication");
    computation_amounts   = new double[] {3e8, 6e8, 1e9}; // 300Mflop, 600Mflop, 1Gflop
    communication_amounts = null;                         // no comm
    this.parallel_execute(hosts, computation_amounts, communication_amounts);

    /* ------[ test 4 ]----------------- */
    Engine.info("Then, build a parallel activity with no computation nor communication (synchro only)");
    this.parallel_execute(hosts, null, null);

    /* ------[ test 5 ]----------------- */
    Engine.info("Then, Monitor the execution of a parallel activity");
    computation_amounts = new double[hosts_count];
    for (int i = 0; i < hosts_count; i++)
      computation_amounts[i] = 1e6 /*1Mflop*/;
    communication_amounts = new double[] {0, 1e6, 0, 0, 0, 1e6, 1e6, 0, 0};
    activity              = this.exec_init(hosts, computation_amounts, communication_amounts);
    activity.start();

    while (!activity.test()) {
      Engine.info("Remaining flop ratio: %.0f%%", 100 * activity.get_remaining_ratio());
      this.sleep_for(5);
    }
    activity.await();

    /* ------[ test 6 ]----------------- */
    Engine.info("Finally, simulate a malleable task (a parallel execution that gets reconfigured after its start).");
    Engine.info("  - Start a regular parallel execution, with both comm and computation");
    computation_amounts = new double[hosts_count];
    for (int i = 0; i < hosts_count; i++)
      computation_amounts[i] = 1e6 /*1Mflop*/;
    communication_amounts = new double[] {0, 1e6, 0, 0, 1e6, 0, 1e6, 0, 0};
    activity              = this.exec_init(hosts, computation_amounts, communication_amounts);
    activity.start();

    this.sleep_for(10);
    double remaining_ratio = activity.get_remaining_ratio();
    Engine.info("  - After 10 seconds, %.2f%% remains to be done. Change it from 3 hosts to 2 hosts only.",
                remaining_ratio * 100);
    Engine.info("    Let's first suspend the task.");
    activity.suspend();

    Engine.info(
        "  - Now, simulate the reconfiguration (modeled as a comm from the removed host to the remaining ones).");
    double[] rescheduling_comp = {0, 0, 0};
    double[] rescheduling_comm = {0, 0, 0, 0, 0, 0, 25000, 25000, 0};
    this.parallel_execute(hosts, rescheduling_comp, rescheduling_comm);

    Engine.info("  - Now, let's cancel the old task and create a new task with modified comm and computation vectors:");
    Engine.info(
        "    What was already done is removed, and the load of the removed host is shared between remaining ones.");
    for (int i = 0; i < 2; i++) {
      // remove what we've done so far, for both comm and compute load
      computation_amounts[i] *= remaining_ratio;
      communication_amounts[i] *= remaining_ratio;
      // The work from 1 must be shared between 2 remaining ones. 1/2=50% of extra work for each
      computation_amounts[i] *= 1.5;
      communication_amounts[i] *= 1.5;
    }
    Host[] newhosts       = new Host[] {hosts[0], hosts[1]};
    double[] newcomp      = new double[] {computation_amounts[0], computation_amounts[1]};
    double remaining_comm = communication_amounts[1];
    communication_amounts =
        new double[] {0, remaining_comm, remaining_comm, 0}; // Resizing a linearized matrix is hairly

    activity.cancel();
    activity = this.exec_init(newhosts, newcomp, communication_amounts);

    Engine.info("  - Done, let's wait for the task completion");
    activity.await();

    Engine.info("Goodbye now!");
  }
}

public class exec_ptask {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    if (args.length < 1)
      Engine.die("Usage: exec_ptask <platform file>");

    e.load_platform(args[0]);
    e.host_by_name("MyHost1").add_actor("test", new runner());

    e.run();
    Engine.info("Simulation done.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
