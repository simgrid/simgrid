/* Copyright (c) 2006-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import java.util.concurrent.Semaphore;
import org.simgrid.s4u.*;

/// Wait on the barrier then leave
class worker extends Actor {
  Barrier barrier;
  public worker(String name, Host location, Barrier barrier)
  {
    super(name, location);
    this.barrier = barrier;
  }
  public void run()
  {
    Engine.info("Waiting on the barrier");
    if (barrier.await())
      Engine.info("Bye from the last to enter");
    else
      Engine.info("Bye");
  }
}

/// Spawn actor_count-1 workers and do a barrier with them
class master extends Actor {
  int actor_count;
  public master(String name, Host location, int actor_count)
  {
    super(name, location);
    this.actor_count = actor_count;
  }
  public void run()
  {
    Barrier barrier = Barrier.create(actor_count);

    Engine.info("Spawning %d workers", actor_count - 1);
    for (int i = 0; i < actor_count - 1; i++)
      new worker("worker", Host.by_name("Jupiter"), barrier);

    Engine.info("Waiting on the barrier");
    if (barrier.await())
      Engine.info("Bye from the last to enter");
    else
      Engine.info("Bye");
  }
}
public class synchro_barrier {
  public static void main(String[] args)
  {
    var e = new Engine(args);

    // Parameter: Number of actors in the barrier
    if (args.length < 1)
      Engine.die("Usage: synchro_barrier <actor-count>\n");

    int actor_count = Integer.valueOf(args[0]);
    if (actor_count <= 0)
      Engine.die("<actor-count> must be greater than 0");

    e.load_platform(args.length >= 2 ? args[1] : "../platforms/two_hosts.xml");
    new master("master", e.host_by_name("Tremblay"), actor_count);
    e.run();
  }
}
