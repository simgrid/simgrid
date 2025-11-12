/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class worker extends Actor {
  public void run() throws SimgridException
  {
    // Define an amount of work that should take 1 second to execute.
    double computation_amount = this.get_host().get_speed();

    // Create an unassigned activity and start it. It will not actually start, because it's not assigned to any host yet
    Exec exec = Exec.init().set_flops_amount(computation_amount).set_name("exec").start();

    // Wait for a while
    this.sleep_for(10);

    // Assign the activity to the current host. This triggers its start, then waits for it completion.
    exec.set_host(this.get_host()).await();
    Engine.info("Exec '%s' is complete", exec.get_name());
  }
}

public class exec_unassigned {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    e.host_by_name("Fafard").add_actor("worker", new worker());

    e.run();

    Engine.info("Simulation ends.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
