/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

public class dag_simple {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);
    e.track_vetoed_activities();

    var fafard = e.host_by_name("Fafard");

    // Display the details on vetoed activities
    Exec.on_veto_cb(new CallbackExec() {
      public void run(Exec exec)
      {
        Engine.info("Execution '%s' vetoed. Dependencies: %s; Ressources: %s", exec.get_name(),
                    (exec.dependencies_solved() ? "solved" : "NOT solved"),
                    (exec.is_assigned() ? "assigned" : "NOT assigned"));
      }
    });

    Exec.on_completion_cb(new CallbackExec() {
      public void run(Exec exec)
      {
        Engine.info("Execution '%s' is complete (start time: %f, finish time: %f)", exec.get_name(),
                    exec.get_start_time(), exec.get_finish_time());
      }
    });

    // Define an amount of work that should take 1 second to execute.
    double computation_amount = fafard.get_speed();

    // Create a small DAG: Two parents and a child
    Exec first_parent  = Exec.init();
    Exec second_parent = Exec.init();
    Exec child         = Exec.init();
    first_parent.add_successor(child);
    second_parent.add_successor(child);

    // Set the parameters (the name is for logging purposes only)
    // + First parent ends after 1 second and the Second parent after 2 seconds.
    first_parent.set_name("parent 1").set_flops_amount(computation_amount);
    second_parent.set_name("parent 2").set_flops_amount(2 * computation_amount);
    child.set_name("child").set_flops_amount(computation_amount);

    // Only the parents are scheduled so far
    first_parent.set_host(fafard);
    second_parent.set_host(fafard);

    // Start all activities that can actually start.
    first_parent.start();
    second_parent.start();
    child.start();

    while (child.get_state() != Activity.State.FINISHED) {
      e.run();
      var vetoed = e.get_vetoed_activities();
      for (var a : vetoed) {
        var exec = (Exec)a;

        // In this simple case, we just assign the child task to a resource when its dependencies are solved
        if (exec.dependencies_solved() && !exec.is_assigned()) {
          Engine.info("Activity %s's dependencies are resolved. Let's assign it to Fafard.", exec.get_name());
          exec.set_host(fafard);
        } else {
          Engine.info("Activity %s not ready.", exec.get_name());
        }
      }
      e.clear_vetoed_activities(); // DON'T FORGET TO CLEAR this set between two calls to run
    }

    Engine.info("Simulation ends.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}