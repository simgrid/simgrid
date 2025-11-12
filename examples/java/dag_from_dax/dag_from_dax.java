/* simple test trying to load a DAX file.                                   */

/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

public class dag_from_dax {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    Activity[] dag = e.create_DAG_from_DAX(args[1]);

    if (dag.length == 0) {
      Engine.error("A problem occurred during DAX parsing (cycle or syntax). Do not continue this test");
      System.exit(2);
    }

    Engine.info("--------- Display all activities of the loaded DAG -----------");
    for (var a : dag) {
      String type = "an Exec";
      String task = "flops to execute";
      if (a instanceof Comm) {
        type = "a Comm";
        task = "bytes to transfer";
      }
      Engine.info("'%s' is %s: %.0f %s. Dependencies: %s; Ressources: %s", a.get_name(), type, a.get_remaining(), task,
                  (a.dependencies_solved() ? "solved" : "NOT solved"), (a.is_assigned() ? "assigned" : "NOT assigned"));
    }

    Engine.info("------------------- Schedule tasks ---------------------------");
    var hosts  = e.get_all_hosts();
    int count  = (int)e.get_host_count();
    int cursor = 0;
    // Schedule end first

    ((Exec)dag[dag.length - 1]).set_host(hosts[0]);

    for (var a : dag) {
      if (a instanceof Exec && a != null && a.get_name() != "end") {
        ((Exec)a).set_host(hosts[cursor % count]);
        cursor++;
      }
      if (a instanceof Comm) {
        Comm comm = (Comm)a;
        var pred  = ((Exec)(comm.get_dependencies()[0]));
        var succ  = ((Exec)comm.get_successors()[0]);
        comm.set_source(pred.get_host()).set_destination(succ.get_host());
      }
    }

    Engine.info("------------------- Run the schedule -------------------------");
    e.run();

    Engine.info("-------------- Summary of executed schedule ------------------");
    for (var a : dag) {
      if (a instanceof Exec) {
        var exec = (Exec)a;
        Engine.info("[%f->%f] '%s' executed on %s", exec.get_start_time(), exec.get_finish_time(), exec.get_name(),
                    exec.get_host().get_name());
      }
      if (a instanceof Comm) {
        var comm = (Comm)a;

        Engine.info("[%f->%f] '%s' transferred from %s to %s", comm.get_start_time(), comm.get_finish_time(),
                    comm.get_name(), comm.get_source().get_name(), comm.get_destination().get_name());
      }
    }

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
