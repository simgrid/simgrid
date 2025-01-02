/* Copyright (c) 2006-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import java.util.Vector;
import org.simgrid.s4u.*;

/* This worker uses a classical mutex */
class worker extends Actor {
  Mutex mutex;
  int idx;
  public worker(String name, Host location, Mutex mutex, int idx)
  {
    super(name, location);

    this.mutex = mutex;
    this.idx   = idx;
  }
  public void run()
  {
    // lock the mutex before enter in the critical section
    mutex.lock();

    Engine.info("Hello Java, I'm ready to compute after locking.");
    // And finally add it to the results
    synchro_mutex.result.set(idx, synchro_mutex.result.get(idx) + 1);
    Engine.info("I'm done, good bye");

    // You have to unlock the mutex manually.
    // Beware of exceptions preventing your unlock() from being executed!
    mutex.unlock();
  }
}

public class synchro_mutex {
  static int cfg_actor_count    = 6;
  static Vector<Integer> result = new Vector<>(cfg_actor_count);

  public static void main(String[] args)
  {
    var e = Engine.get_instance(args);
    e.load_platform(args[0]);

    /* Create the requested amount of actors pairs. Each pair has a specific mutex and cell in `result`. */
    for (int i = 0; i < cfg_actor_count; i++) {
      result.add(0);
      Mutex mutex = Mutex.create();
      new worker("worker", Host.by_name("Jupiter"), mutex, i);
      new worker("worker", Host.by_name("Tremblay"), mutex, i);
    }

    e.run();

    for (int i = 0; i < cfg_actor_count; i++)
      Engine.info("Result[%d] -> %d", i, result.get(i));
  }
}
