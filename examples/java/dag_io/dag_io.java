/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

public class dag_io {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    var bob  = e.host_by_name("bob");
    var carl = e.host_by_name("carl");

    // Display the details on vetoed activities
    Exec.on_veto_cb(new CallbackExec() {
      @Override public void run(Exec exec)
      {
        Engine.info("Exec '%s' vetoed. Dependencies: %s; Ressources: %s", exec.get_name(),
                    (exec.dependencies_solved() ? "solved" : "NOT solved"),
                    (exec.is_assigned() ? "assigned" : "NOT assigned"));
      }
    });
    Io.on_veto_cb(new CallbackIo() {
      public void run(Io io)
      {
        Engine.info("Io '%s' vetoed. Dependencies: %s; Ressources: %s", io.get_name(),
                    (io.dependencies_solved() ? "solved" : "NOT solved"),
                    (io.is_assigned() ? "assigned" : "NOT assigned"));
      }
    });

    Exec.on_completion_cb(new CallbackExec() {
      public void run(Exec exec)
      {
        Engine.info("Exec '%s' is complete (start time: %f, finish time: %f)", exec.get_name(), exec.get_start_time(),
                    exec.get_finish_time());
      }
    });

    // Create a small DAG: parent.write_output.read_input.child
    Exec parent     = Exec.init();
    Io write_output = Io.init();
    Io read_input   = Io.init();
    Exec child      = Exec.init();
    parent.add_successor(write_output);
    write_output.add_successor(read_input);
    read_input.add_successor(child);

    // Set the parameters (the name is for logging purposes only)
    // + parent and chile end after 1 second
    parent.set_name("parent").set_flops_amount(bob.get_speed());
    write_output.set_name("write").set_size(1e9).set_op_type(Io.OpType.WRITE);
    read_input.set_name("read").set_size(1e9).set_op_type(Io.OpType.READ);
    child.set_name("child").set_flops_amount(carl.get_speed());

    // Schedule and try to start the different activities
    parent.set_host(bob).start();
    write_output.set_disk(bob.get_disks()[0]).start();
    read_input.set_disk(carl.get_disks()[0]).start();
    child.set_host(carl).start();

    e.run();

    Engine.info("Simulation ends.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
