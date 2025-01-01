/* Copyright (c) 2017-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class sleeper extends ActorMain {
  public void run()
  {
    Engine.info("Sleeper started");
    sleep_for(3);
    Engine.info("I'm done. See you!");
  }
}

class master extends ActorMain {
  public void run()
  {
    Actor actor;

    Engine.info("Start sleeper");
    actor = Actor.create("sleeper from master", Host.current(), new sleeper());
    Engine.info("Join the sleeper (timeout 2)");
    actor.join(2);

    Engine.info("Start sleeper");
    actor = Actor.create("sleeper from master", Host.current(), new sleeper());
    Engine.info("Join the sleeper (timeout 4)");
    actor.join(4);

    Engine.info("Start sleeper");
    actor = Actor.create("sleeper from master", Host.current(), new sleeper());
    Engine.info("Join the sleeper (timeout 2)");
    actor.join(2);

    Engine.info("Start sleeper");
    actor = Actor.create("sleeper from master", Host.current(), new sleeper());
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
    var e = Engine.get_instance(args);

    e.load_platform(args[0]);

    Actor.create("master", e.host_by_name("Tremblay"), new master());

    e.run();

    Engine.info("Simulation ends.");
  }
}
