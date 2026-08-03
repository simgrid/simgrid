/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Test extends Actor {
  public void run() throws SimgridException
  {
    Exec bob_compute  = this.exec_init(1e9);
    Io bob_write      = this.get_host().get_disks()[0].io_init(4000000, Io.OpType.WRITE);
    Io carl_read      = this.get_engine().host_by_name("carl").get_disks()[0].io_init(4000000, Io.OpType.READ);
    Exec carl_compute = this.get_engine().host_by_name("carl").exec_init(1e9);

    ActivitySet pending_activities = new ActivitySet();
    pending_activities.push(bob_compute);
    pending_activities.push(bob_write);
    pending_activities.push(carl_read);
    pending_activities.push(carl_compute);

    // Name the activities (for logging purposes only)
    bob_compute.set_name("bob compute");
    bob_write.set_name("bob write");
    carl_read.set_name("carl read");
    carl_compute.set_name("carl compute");

    // Create the dependencies:
    // 'bob_write' is a successor of 'bob_compute'
    // 'carl_read' is a successor of 'bob_write'
    // 'carl_compute' is a successor of 'carl_read'
    bob_compute.add_successor(bob_write);
    bob_write.add_successor(carl_read);
    carl_read.add_successor(carl_compute);

    // Start the activities.
    bob_compute.start();
    bob_write.start();
    carl_read.start();
    carl_compute.start();

    // wait for the completion of all activities
    while (!pending_activities.empty()) {
      Activity completed_one = pending_activities.await_any();
      if (completed_one != null)
        Engine.info("Activity '%s' is complete", completed_one.get_name());
    }
  }
}

public class io_dependent {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    e.host_by_name("bob").add_actor("bob", new Test());

    e.run();

    Engine.info("Simulation time %g", Engine.get_clock());

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
