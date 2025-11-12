/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

/* The worker actor, working for a while before leaving */
class worker extends Actor {
  public void run()
  {
    Engine.info("Let's do some work (for 10 sec on Boivin).");
    execute(980.95e6);

    Engine.info("I'm done now. I leave even if it makes the daemon die.");
  }
}

/* The daemon, displaying a message every 3 seconds until all other actors stop */
class my_daemon extends Actor {
  public void run()
  {
    Actor.self().daemonize();

    while (get_host().is_on()) {
      Engine.info("Hello from the infinite loop");
      sleep_for(3.0);
    }

    Engine.info("I will never reach that point: daemons are killed when regular actors are done");
  }
}

public class actor_daemon {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);
    e.host_by_name("Boivin").add_actor("worker", new worker());
    e.host_by_name("Tremblay").add_actor("daemon", new my_daemon());

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
