/* Copyright (c) 2019-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class worker extends Actor {
  public void run() throws HostFailureException
  {
    Exec exec;
    double amount = 5 * this.get_host().get_speed();
    Engine.info("Create an activity that should run for 5 seconds");

    exec = this.exec_async(amount);

    /* Now that execution is started, wait for 3 seconds. */
    Engine.info("But let it end after 3 seconds");
    try {
      exec.await_for(3);
      Engine.info("Execution complete");
    } catch (TimeoutException e) {
      Engine.info("Execution Wait Timeout!");
    }

    /* do it again, but this time with a timeout greater than the duration of the execution */
    Engine.info("Create another activity that should run for 5 seconds and await for it for 6 seconds");
    exec = this.exec_async(amount);
    try {
      exec.await_for(6);
      Engine.info("Execution complete");
    } catch (TimeoutException e) {
      Engine.info("Execution Wait Timeout!");
    }

    Engine.info("Finally test with a parallel execution");
    var hosts                      = this.get_engine().get_all_hosts();
    int hosts_count                = hosts.length;
    double[] computation_amounts   = new double[hosts_count];
    double[] communication_amounts = new double[hosts_count * hosts_count];

    for (int i = 0; i < hosts_count; i++) {
      computation_amounts[i] = 1e9 /*1Gflop*/;
      for (int j = i + 1; j < hosts_count; j++)
        communication_amounts[i * hosts_count + j] = 1e7; // 10 MB
    }

    exec = this.exec_init(hosts, computation_amounts, communication_amounts);
    try {
      exec.await_for(2);
      Engine.info("Parallel Execution complete");
    } catch (TimeoutException e) {
      Engine.info("Parallel Execution Wait Timeout!");
    }
  }
}

public class exec_awaitfor {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);
    e.host_by_name("Tremblay").add_actor("worker", new worker());
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
