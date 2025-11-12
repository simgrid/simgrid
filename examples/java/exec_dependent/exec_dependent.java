/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Worker extends Actor {
  public void run() throws SimgridException
  {

    // Define an amount of work that should take 1 second to execute.
    double computation_amount = get_host().get_speed();

    // Create a small DAG
    // + Two parents and a child
    // + First parent ends after 1 second and the Second parent after 2 seconds.
    Exec first_parent  = exec_init(computation_amount);
    Exec second_parent = exec_init(2 * computation_amount);
    Exec child         = Exec.init().set_flops_amount(computation_amount);

    ActivitySet pending_execs = new ActivitySet();
    pending_execs.push(first_parent);
    pending_execs.push(second_parent);
    pending_execs.push(child);

    // Name the activities (for logging purposes only)
    first_parent.set_name("parent 1");
    second_parent.set_name("parent 2");
    child.set_name("child");

    // Create the dependencies by declaring 'child' as a successor of first_parent and second_parent
    first_parent.add_successor(child);
    second_parent.add_successor(child);

    // Start the activities.
    first_parent.start();
    second_parent.start();
    child.start();

    // wait for the completion of all activities
    while (!pending_execs.empty()) {
      Activity completed_one = pending_execs.await_any();
      if (completed_one != null)
        Engine.info("Exec '%s' is complete", completed_one.get_name());
    }
  }
}

public class exec_dependent {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    e.host_by_name("Fafard").add_actor("worker", new Worker());

    Exec.on_veto_cb(new CallbackExec() {
      public void run(Exec exec)
      {
        // First display the situation
        Engine.info("Activity '%s' vetoed. Dependencies: %s; Ressources: %s", exec.get_name(),
                    (exec.dependencies_solved() ? "solved" : "NOT solved"),
                    (exec.is_assigned() ? "assigned" : "NOT assigned"));

        // In this simple case, we just assign the child task to a resource when its dependencies are solved
        if (exec.dependencies_solved() && !exec.is_assigned()) {
          Engine.info("Activity %s's dependencies are resolved. Let's assign it to Fafard.", exec.get_name());
          exec.set_host(e.host_by_name("Fafard"));
        }
      }
    });

    e.run();

    Engine.info("Simulation ends.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
