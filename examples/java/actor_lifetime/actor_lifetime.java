/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This C++ file acts as the foil to the corresponding XML file, where the
   action takes place: Actors are started and stopped at predefined time.   */

import org.simgrid.s4u.*;

/* This actor just sleeps until termination */
class sleeper extends Actor {
  public sleeper(String[] args)
  {
    this.on_exit(new CallbackBoolean() {
      @Override public void run(boolean failed)
      {
        /* Executed on actor termination, to display a message helping to understand the output */
        Engine.info("Exiting now (done sleeping or got killed).");
      }
    });
  }
  public void run()
  {
    Engine.info("Hello! I go to sleep.");
    this.sleep_for(10);
    Engine.info("Done sleeping.");
  }
}

public class actor_lifetime {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);   /* Load the platform description */
    e.load_deployment(args[1]); /*  Deploy the sleeper actors with explicit start/kill times */

    e.run(); /* - Run the simulation */

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
