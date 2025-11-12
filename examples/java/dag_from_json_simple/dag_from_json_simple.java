/* Copyright (c) 2003-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

public class dag_from_json_simple {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    e.create_DAG_from_json(args[1]);

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
        Engine.info("Comm '%s' is complete (start time: %f, finish time: %f)", comm.get_name(), comm.get_start_time(),
                    comm.get_finish_time());
      }
    });

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}