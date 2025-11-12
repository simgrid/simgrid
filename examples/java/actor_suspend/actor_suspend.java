/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

/* The Lazy guy only wants to sleep, but can be awaken by the dream_master actor. */
class LazyGuy extends Actor {
  public void run()
  {
    Engine.info("Nobody's watching me ? Let's go to sleep.");
    this.suspend(); /* - Start by suspending itself */
    Engine.info("Uuuh ? Did somebody call me ?");

    Engine.info("Going to sleep..."); /* - Then repetitively go to sleep, but got awaken */
    this.sleep_for(10);
    Engine.info("Mmm... waking up.");

    Engine.info("Going to sleep one more time (for 10 sec)...");
    this.sleep_for(10);
    Engine.info("Waking up once for all!");

    Engine.info("Ok, let's do some work, then (for 10 sec on Boivin).");
    this.execute(980.95e6);

    Engine.info("Mmmh, I'm done now. Goodbye.");
  }
}

/* The Dream master: */
class DreamMaster extends Actor {

  public void run()
  {
    Engine.info("Let's create a lazy guy."); /* - Create a lazy_guy actor */
    Actor lazy = this.get_host().add_actor("Lazy", new LazyGuy());
    Engine.info("Let's wait a little bit...");
    this.sleep_for(10); /* - Wait for 10 seconds */
    Engine.info("Let's wake the lazy guy up! >:) BOOOOOUUUHHH!!!!");
    if (lazy.is_suspended())
      lazy.resume(); /* - Then wake up the lazy_guy */
    else
      Engine.error("I was thinking that the lazy guy would be suspended now");

    this.sleep_for(5); /* Repeat two times: */
    Engine.info("Suspend the lazy guy while he's sleeping...");
    lazy.suspend(); /* - Suspend the lazy_guy while he's asleep */
    Engine.info("Let him finish his siesta.");
    this.sleep_for(10); /* - Wait for 10 seconds */
    Engine.info("Wake up, lazy guy!");
    lazy.resume(); /* - Then wake up the lazy_guy again */

    this.sleep_for(5);
    Engine.info("Suspend again the lazy guy while he's sleeping...");
    lazy.suspend();
    Engine.info("This time, don't let him finish his siesta.");
    this.sleep_for(2);
    Engine.info("Wake up, lazy guy!");
    lazy.resume();

    this.sleep_for(5);
    Engine.info("Give a 2 seconds break to the lazy guy while he's working...");
    lazy.suspend();
    this.sleep_for(2);
    Engine.info("Back to work, lazy guy!");
    lazy.resume();

    Engine.info("OK, I'm done here.");
  }
}
public class actor_suspend {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);
    e.host_by_name("Boivin").add_actor("dream_master", new DreamMaster());
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
