/* Copyright (c) 2003-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

public class dag_tuto {
  public static void main(String[] args)
  {

    Engine e = new Engine(args);
    e.load_platform(args[0]);

    Host tremblay = e.host_by_name("Tremblay");
    Host jupiter  = e.host_by_name("Jupiter");

    Exec c1 = Exec.init();
    Exec c2 = Exec.init();
    Exec c3 = Exec.init();
    Comm t1 = Comm.sendto_init();

    c1.set_name("c1");
    c2.set_name("c2");
    c3.set_name("c3");
    t1.set_name("t1");

    c1.set_flops_amount(1e9);
    c2.set_flops_amount(5e9);
    c3.set_flops_amount(2e9);
    t1.set_payload_size(5e8);

    c1.add_successor(t1);
    t1.add_successor(c3);
    c2.add_successor(c3);

    c1.set_host(tremblay);
    c2.set_host(jupiter);
    c3.set_host(jupiter);
    t1.set_source(tremblay);
    t1.set_destination(jupiter);

    c1.start();
    c2.start();

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