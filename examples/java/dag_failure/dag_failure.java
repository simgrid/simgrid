/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

public class dag_failure {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    Engine.set_config("host/model:ptask_L07");
    e.load_platform(args[0]);

    var faulty = e.host_by_name("Faulty Host");
    var safe   = e.host_by_name("Safe Host");
    Exec.on_completion_cb(new CallbackExec() {
      public void run(Exec exec)
      {
        if (exec.get_state() == Activity.State.FINISHED)
          Engine.info("Activity '%s' is complete (start time: %f, finish time: %f)", exec.get_name(),
                      exec.get_start_time(), exec.get_finish_time());
        if (exec.get_state() == Activity.State.FAILED) {
          if (exec.is_parallel())
            Engine.info("Activity '%s' has failed. %.0f %% remain to be done", exec.get_name(),
                        100 * exec.get_remaining_ratio());
          else
            Engine.info("Activity '%s' has failed. %.0f flops remain to be done", exec.get_name(),
                        exec.get_remaining());
        }
      }
    });

    /* creation of a single Exec that will poorly fail when the workstation will stop */
    Engine.info("First test: sequential Exec activity");
    Exec exec = Exec.init().set_name("Poor task").set_flops_amount(2e10).start();

    Engine.info("Schedule Activity '%s' on 'Faulty Host'", exec.get_name());
    exec.set_host(faulty);

    /* Add a child Exec that depends on the Poor task' */
    Exec child = Exec.init().set_name("Child").set_flops_amount(2e10).set_host(safe);
    exec.add_successor(child);
    child.start();

    Engine.info("Run the simulation");
    e.run();

    Engine.info("let's unschedule Activity '%s' and reschedule it on the 'Safe Host'", exec.get_name());
    exec.unset_host();
    exec.set_host(safe);

    Engine.info("Run the simulation again");
    e.run();

    Engine.info("Second test: parallel Exec activity");
    exec = Exec.init().set_name("Poor parallel task").set_flops_amounts(new double[] {2e10, 2e10}).start();

    Engine.info("Schedule Activity '%s' on 'Safe Host' and 'Faulty Host'", exec.get_name());
    exec.set_hosts(new Host[] {safe, faulty});

    /* Add a child Exec that depends on the Poor parallel task' */
    child = Exec.init().set_name("Child").set_flops_amount(2e10).set_host(safe);
    exec.add_successor(child);
    child.start();

    Engine.info("Run the simulation");
    e.run();

    Engine.info("let's unschedule Activity '%s' and reschedule it only on the 'Safe Host'", exec.get_name());
    exec.unset_hosts();
    exec.set_flops_amount(4e10).set_host(safe);

    Engine.info("Run the simulation again");
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
