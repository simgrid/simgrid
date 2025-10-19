/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class dvfs extends Actor {
  public void run()
  {
    double workload = 100E6;
    Host host       = this.get_host();

    int nb = host.get_pstate_count();
    Engine.info("Count of Processor states=%d", nb);

    Engine.info("Current power peak=%f", host.get_speed());

    // Run a Computation
    this.execute(workload);

    double exec_time = Engine.get_clock();
    Engine.info("Computation1 duration: %.2f", exec_time);

    // Change power peak
    int new_pstate = 2;

    Engine.info("Changing power peak value to %f (at index %d)", host.get_pstate_speed(new_pstate), new_pstate);

    host.set_pstate(new_pstate);

    Engine.info("Current power peak=%f", host.get_speed());

    // Run a second Computation
    this.execute(workload);

    exec_time = Engine.get_clock() - exec_time;
    Engine.info("Computation2 duration: %.2f", exec_time);

    // Verify that the default pstate is set to 0
    host = get_engine().host_by_name("MyHost2");
    Engine.info("Count of Processor states=%d", host.get_pstate_count());

    Engine.info("Current power peak=%f", host.get_speed());
  }
}

public class exec_dvfs {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    if (args.length == 1)
      Engine.die("Usage: %s platform_file\n\tExample: exec_dvfs ../platforms/energy_platform.xml\n");

    e.load_platform(args[0]);

    e.host_by_name("MyHost1").add_actor("dvfs_test", new dvfs());
    e.host_by_name("MyHost2").add_actor("dvfs_test", new dvfs());

    e.run();

    Engine.info("Total simulation time: %e", Engine.get_clock());

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
