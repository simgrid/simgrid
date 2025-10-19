/* Copyright (c) 2003-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

public class dag_from_dot_simple {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    Activity[] dag = e.create_DAG_from_dot(args[1]);

    Host tremblay = e.host_by_name("Tremblay");
    Host jupiter  = e.host_by_name("Jupiter");
    Host fafard   = e.host_by_name("Fafard");

    ((Exec)dag[0]).set_host(fafard);
    ((Exec)dag[1]).set_host(tremblay);
    ((Exec)dag[2]).set_host(jupiter);
    ((Exec)dag[3]).set_host(jupiter);
    ((Exec)dag[8]).set_host(jupiter);

    for (var a : dag) {
      if (a instanceof Comm) {
        var comm = (Comm)a;
        var pred = (Exec)comm.get_dependencies()[0];
        var succ = (Exec)comm.get_successors()[0];
        comm.set_source(pred.get_host()).set_destination(succ.get_host());
      }
    }

    Exec.on_completion_cb(new CallbackExec() {
      public void run(Exec exec)
      {
        Engine.info("Exec '%s' on %s is complete (start time: %f, finish time: %f)", exec.get_name(),
                    exec.get_host().get_name(), exec.get_start_time(), exec.get_finish_time());
      }
    });

    Comm.on_completion_cb(new CallbackComm() {
      public void run(Comm comm)
      {
        Engine.info("Comm '%s' from %s to %s is complete (start time: %f, finish time: %f)", comm.get_name(),
                    comm.get_source().get_name(), comm.get_destination().get_name(), comm.get_start_time(),
                    comm.get_finish_time());
      }
    });

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}