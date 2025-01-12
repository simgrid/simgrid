/* Copyright (c) 2017-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class VictimA extends Actor {
  public void run()
  {
    on_exit(new CallbackBoolean() {
      @Override public void run(boolean failed)
      {
        Engine.info("I have been killed!");
      }
    });
    Engine.info("Hello!");
    Engine.info("Suspending myself");
    suspend();                         /* - Start by suspending itself */
    Engine.info("OK, OK. Let's work"); /* - Then is resumed and start to execute some flops */
    execute(1e9);
    Engine.info("Bye!"); /* - But will never reach the end of it */
  }
}

class VictimB extends Actor {
  public void run() { Engine.info("Terminate before being killed"); }
}

class Killer extends Actor {
  public void run()
  {
    var e = this.get_engine();
    Engine.info("Hello!"); /* - First start a victim actor */
    Actor victimA = e.add_actor("victim A", e.host_by_name("Fafard"), new VictimA());
    Actor victimB = e.add_actor("victim B", e.host_by_name("Jupiter"), new VictimB());
    sleep_for(10); /* - Wait for 10 seconds */

    Engine.info("Resume the victim A"); /* - Resume it from its suspended state */
    victimA.resume();
    sleep_for(2);

    Engine.info("Kill the victim A");       /* - and then kill it */
    Actor.by_pid(victimA.get_pid()).kill(); // You can retrieve an actor from its PID (and then kill it)

    sleep_for(1);

    Engine.info("Kill victimB, even if it's already dead"); /* that's a no-op, there is no zombies in SimGrid */
    victimB.kill(); // the actor is automatically garbage-collected after this last reference

    sleep_for(1);

    Engine.info("Start a new actor, and kill it right away.");
    Actor victimC = e.add_actor("victim C", e.host_by_name("Jupiter"), new VictimA());
    victimC.kill();

    sleep_for(1);

    Engine.info("Killing everybody but myself");
    Actor.kill_all();

    Engine.info("OK, goodbye now. I commit a suicide.");
    exit();

    Engine.info("This line never gets displayed: I'm already dead since the previous line.");
  }
}

public class actor_kill {
  public static void main(String[] args)
  {
    var e = new Engine(args);

    e.load_platform(args[0]); /* - Load the platform description */
    /* - Create and deploy killer actor, that will create the victim actors  */
    e.add_actor("killer", e.host_by_name("Tremblay"), new Killer());

    e.run(); /* - Run the simulation */
  }
}
