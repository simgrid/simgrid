/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class sleeper extends Actor {
  public void run()
  {
    Engine.info("Sleeper started");
    sleep_for(3);
    Engine.info("I'm done. See you!");
  }
}

class master extends Actor {
  public void run()
  {
    Actor actor;

    Engine.info("Start sleeper");
    actor = Host.current().add_actor("sleeper from master", new sleeper());
    Engine.info("Join the sleeper (timeout 2)");
    actor.join(2);

    Engine.info("Start sleeper");
    actor = Host.current().add_actor("sleeper from master", new sleeper());
    Engine.info("Join the sleeper (timeout 4)");
    actor.join(4);

    Engine.info("Start sleeper");
    actor = Host.current().add_actor("sleeper from master", new sleeper());
    Engine.info("Join the sleeper (timeout 2)");
    actor.join(2);

    Engine.info("Start sleeper");
    actor = Host.current().add_actor("sleeper from master", new sleeper());
    Engine.info("Waiting 4");
    sleep_for(4);
    Engine.info("Join the sleeper after its end (timeout 1)");
    actor.join(1);

    Engine.info("Goodbye now!");

    sleep_for(1);

    Engine.info("Goodbye now!");
  }
}

public class actor_join {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);
    e.host_by_name("Tremblay").add_actor("master", new master());
    e.run();

    Engine.info("Simulation ends.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
