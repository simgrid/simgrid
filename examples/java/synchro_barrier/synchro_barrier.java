/* Copyright (c) 2006-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

/// Wait on the barrier then leave
class worker extends ActorMain {
  Barrier barrier;
  public worker(Barrier barrier_) { barrier = barrier_; }
  public void run()
  {
    Engine.info("Waiting on the barrier");
    barrier.await();

    Engine.info("Bye");
  }
}

/// Spawn actor_count-1 workers and do a barrier with them
class master extends ActorMain {
  int actor_count;
  master(int actor_count) { this.actor_count = actor_count; }
  public void run()
  {
    Barrier barrier = Barrier.create(actor_count);

    Engine.info("Spawning %d workers", actor_count - 1);
    for (int i = 0; i < actor_count - 1; i++)
      Actor.create("worker", Host.by_name("Jupiter"), new worker(barrier));

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
    var e = Engine.get_instance(args);

    // Parameter: Number of actors in the barrier
    if (args.length < 1)
      Engine.die("Usage: synchro_barrier <actor-count>\n");

    int actor_count = Integer.valueOf(args[0]);
    if (actor_count <= 0)
      Engine.die("<actor-count> must be greater than 0");

    e.load_platform(args.length >= 2 ? args[1] : "../platforms/two_hosts.xml");
    Actor.create("master", e.host_by_name("Tremblay"), new master(actor_count));
    e.run();
  }
}
