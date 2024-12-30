/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

/* This actor simply starts an activity in a fire and forget (a.k.a. detached) mode and quit.*/
class detached extends ActorMain {
  public void run()
  {
    double computation_amount = get_host().get_speed();
    Engine.info("Execute %g flops, should take 1 second.", computation_amount);
    Exec activity = exec_init(computation_amount);
    // Attach a callback to print some log when the detached activity completes
    activity.on_this_completion_cb(new CallbackExec() {
      public void run(Exec e)
      {
        Engine.info("Detached activity is done");
      }
    });

    activity.detach();

    Engine.info("Goodbye now!");
  }
}

/* This actor simply waits for its activity completion after starting it.
 * That's exactly equivalent to synchronous execution. */
class waiter extends ActorMain {
  public void run()
  {
    double computation_amount = get_host().get_speed();
    Engine.info("Execute %g flops, should take 1 second.", computation_amount);
    Exec activity = exec_init(computation_amount);
    activity.start();
    activity.await();

    Engine.info("Goodbye now!");
  }
}

/* This actor tests the ongoing execution until its completion, and don't wait before it's terminated. */
class monitor extends ActorMain {
  public void run()
  {
    double computation_amount = get_host().get_speed();
    Engine.info("Execute %g flops, should take 1 second.", computation_amount);
    Exec activity = exec_init(computation_amount);
    activity.start();

    while (!activity.test()) {
      Engine.info("Remaining amount of flops: %g (%.0f%%)", activity.get_remaining(),
                  100 * activity.get_remaining_ratio());
      sleep_for(0.3);
    }

    Engine.info("Goodbye now!");
  }
}

/* This actor cancels the ongoing execution after a while. */
class canceller extends ActorMain {
  public void run()
  {
    double computation_amount = get_host().get_speed();

    Engine.info("Execute %g flops, should take 1 second.", computation_amount);
    Exec activity = exec_async(computation_amount);
    sleep_for(0.5);
    Engine.info("I changed my mind, cancel!");
    activity.cancel();

    Engine.info("Goodbye now!");
  }
}

public class exec_async {
  public static void main(String[] args)
  {
    var e = Engine.get_instance(args);
    e.load_platform(args[0]);

    Host fafard   = e.host_by_name("Fafard");
    Host ginette  = e.host_by_name("Ginette");
    Host boivin   = e.host_by_name("Boivin");
    Host tremblay = e.host_by_name("Tremblay");

    Actor.create("wait", fafard, new waiter());
    Actor.create("monitor", ginette, new monitor());
    Actor.create("cancel", boivin, new canceller());
    Actor.create("detach", tremblay, new detached());

    e.run();

    Engine.info("Simulation ends.");
  }
}
