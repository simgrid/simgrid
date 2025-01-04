/* Copyright (c) 2017-2024. The SimGrid Team. All rights reserved.          */

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
  long number_of_yields;
  public Yielder(String name, Host location, long number_of_yields)
  {
    super(name, location);
    this.number_of_yields = number_of_yields;
  }
  public void run()
  {
    for (int i = 0; i < number_of_yields; i++)
      this.yield();
    Engine.info("I yielded %d times. Goodbye now!", number_of_yields);
  }
}

public class actor_yield {
  public static void main(String[] args)
  {
    var e = new Engine(args);
    e.load_platform(args[0]);

    new Yielder("yielder", e.host_by_name("Tremblay"), 10);
    new Yielder("yielder", e.host_by_name("Ruby"), 15);

    e.run(); /* - Run the simulation */
  }
}