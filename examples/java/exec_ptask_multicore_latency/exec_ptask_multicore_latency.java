/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class runner extends Actor {
  public void run()
  {
    var e         = this.get_engine();
    double[] comp = new double[] {1e9, 1e9};
    double[] comm = new double[] {0.0, 0.0, 0.0, 0.0};
    // Different hosts.
    Host[] hosts_diff = new Host[] {e.host_by_name("MyHost1"), e.host_by_name("MyHost2")};
    double start_time = Engine.get_clock();
    this.parallel_execute(hosts_diff, comp, comm);
    Engine.info("Computed 2-core activity on two different hosts. Took %.0f s", Engine.get_clock() - start_time);

    // Same host, multicore.
    Host[] multicore_host = new Host[] {e.host_by_name("MyHost1"), e.host_by_name("MyHost1")};
    start_time            = Engine.get_clock();
    this.parallel_execute(multicore_host, comp, comm);
    Engine.info("Computed 2-core activity on one 4-core host. Took %.0f s", Engine.get_clock() - start_time);

    // Same host, using too many cores
    double[] comp6 = new double[6];
    double[] comm6 = new double[36];
    for (int i = 0; i < 6; i++) {
      comp6[i] = 1e9 /*1Gflop*/;
      for (int j = i + 1; j < 6; j++)
        comm6[i * 6 + j] = 0;
    }
    Host h1                   = e.host_by_name("MyHost1");
    Host[] multicore_overload = new Host[] {h1, h1, h1, h1, h1, h1};
    start_time                = Engine.get_clock();
    this.parallel_execute(multicore_overload, comp6, comm6);
    Engine.info("Computed 6-core activity on a 4-core host. Took %.0f s", Engine.get_clock() - start_time);

    // Same host, adding some communication
    double[] comm2 = new double[] {0, 1E7, 1E7, 0};
    start_time     = Engine.get_clock();
    this.parallel_execute(multicore_host, comp, comm2);
    Engine.info("Computed 2-core activity on a 4-core host with some communication. Took %.4f s",
                Engine.get_clock() - start_time);

    // See if the multicore execution continues to work after changing pstate
    Engine.info("Switching machine multicore to pstate 1.");
    e.host_by_name("MyHost1").set_pstate(1);
    Engine.info("Switching back to pstate 0.");
    e.host_by_name("MyHost1").set_pstate(0);

    start_time = Engine.get_clock();
    this.parallel_execute(multicore_host, comp, comm);
    Engine.info("Computed 2-core activity on one 4-core host. Took %.0f s", Engine.get_clock() - start_time);

    start_time = Engine.get_clock();
    this.parallel_execute(hosts_diff, comp, comm);
    Engine.info("Computed 2-core activity on two different hosts. Took %.0f s", Engine.get_clock() - start_time);
  }
}

public class exec_ptask_multicore_latency {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    if (args.length < 1)
      Engine.die("Usage: exec_ptask_multicore_latency <platform file>");

    e.load_platform(args[0]);
    e.host_by_name("MyHost1").add_actor("test", new runner());

    e.run();
    Engine.info("Simulation done.");
  }
}
