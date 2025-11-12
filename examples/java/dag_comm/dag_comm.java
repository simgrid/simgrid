/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

public class dag_comm {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    Host tremblay = e.host_by_name("Tremblay");
    Host jupiter  = e.host_by_name("Jupiter");

    // Display the details on vetoed activities
    Exec.on_veto_cb(new CallbackExec() {
      public void run(Exec exec)
      {
        Engine.info("Execution '%s' vetoed. Dependencies: %s; Ressources: %s", exec.get_name(),
                    (exec.dependencies_solved() ? "solved" : "NOT solved"),
                    (exec.is_assigned() ? "assigned" : "NOT assigned"));
      }
    });
    Comm.on_veto_cb(new CallbackComm() {
      public void run(Comm comm)
      {
        Engine.info("Communication '%s' vetoed. Dependencies: %s; Ressources: %s", comm.get_name(),
                    (comm.dependencies_solved() ? "solved" : "NOT solved"),
                    (comm.is_assigned() ? "assigned" : "NOT assigned"));
      }
    });

    Exec.on_completion_cb(new CallbackExec() {
      public void run(Exec exec)
      {
        Engine.info("Exec '%s' is complete (start time: %f, finish time: %f)", exec.get_name(), exec.get_start_time(),
                    exec.get_finish_time());
      }
    });
    Comm.on_completion_cb(new CallbackComm() {
      public void run(Comm comm)
      {
        Engine.info("Comm '%s' is complete", comm.get_name());
      }
    });

    // Create a small DAG: parent.transfer.child
    Exec parent   = Exec.init();
    Comm transfer = Comm.sendto_init();
    Exec child    = Exec.init();
    parent.add_successor(transfer);
    transfer.add_successor(child);

    // Set the parameters (the name is for logging purposes only)
    // + parent and child end after 1 second
    parent.set_name("parent").set_flops_amount(tremblay.get_speed()).start();
    transfer.set_name("transfer").set_payload_size(125e6).start();
    child.set_name("child").set_flops_amount(jupiter.get_speed()).start();

    // Schedule the different activities
    parent.set_host(tremblay);
    transfer.set_source(tremblay);
    child.set_host(jupiter);
    transfer.set_destination(jupiter);

    e.run();

    Engine.info("Simulation ends.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
