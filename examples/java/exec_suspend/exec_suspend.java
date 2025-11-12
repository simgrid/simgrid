/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class executor extends Actor {
  private void suspend_activity_for(Exec activity, double seconds)
  {
    activity.suspend();
    double remaining_computations_before_sleep = activity.get_remaining();
    this.sleep_for(seconds);
    activity.resume();

    // Sanity check: remaining computations have to be the same before and after this period
    Engine.info("Activity hasn't advanced: %s. (remaining before sleep: %f, remaining after sleep: %f)",
                remaining_computations_before_sleep == activity.get_remaining() ? "yes" : "no",
                remaining_computations_before_sleep, activity.get_remaining());
  }

  public void run() throws SimgridException
  {
    double computation_amount = this.get_host().get_speed();

    // Execution will take 3 seconds
    var activity = this.exec_init(3 * computation_amount);

    // Add functions to be called when the activity is resumed or suspended
    activity.on_this_suspend_cb(new CallbackExec() {
      @Override public void run(Exec t)
      {
        Engine.info("Task is suspended (remaining: %f)", t.get_remaining());
      }
    });

    activity.on_this_resume_cb(new CallbackExec() {
      @Override public void run(Exec t)
      {
        Engine.info("Task is resumed (remaining: %f)", t.get_remaining());
      }
    });

    // The first second of the execution
    activity.start();
    this.sleep_for(1);

    // Suspend the activity for 10 seconds
    suspend_activity_for(activity, 10);

    // Run the activity for one second
    this.sleep_for(1);

    // Suspend the activity for the second time
    suspend_activity_for(activity, 10);

    // Finish execution
    activity.await();
    Engine.info("Finished");
  }
}

public class exec_suspend {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    e.host_by_name("Tremblay").add_actor("executor", new executor());

    // Start the simulation
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
