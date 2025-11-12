/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

/* This example does not much: It just spans over-polite actor that yield a large amount
 * of time before ending.
 *
 * This serves as an example for the sg4::this_actor::yield() function, with which an actor can request
 * to be rescheduled after the other actor that are ready at the current timestamp.
 *
 * It can also be used to benchmark our context-switching mechanism.
 */

class Yielder extends Actor {
  long numberOfYields;
  public Yielder(long numberOfYields) { this.numberOfYields = numberOfYields; }
  public void run()
  {
    for (int i = 0; i < numberOfYields; i++)
      this.yield();
    Engine.info("I yielded %d times. Goodbye now!", numberOfYields);
  }
}

public class actor_yield {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    e.host_by_name("Tremblay").add_actor("yielder", new Yielder(10));
    e.host_by_name("Ruby").add_actor("yielder", new Yielder(15));

    e.run(); /* - Run the simulation */

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}