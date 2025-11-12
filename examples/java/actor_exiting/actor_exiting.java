/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* There is two very different ways of being informed when an actor exits.
 *
 * The this_actor::on_exit() function allows one to register a function to be
 * executed when this very actor exits. The registered function will run
 * when this actor terminates (either because its main function returns, or
 * because it's killed in any way). No simcall are allowed here: your actor
 * is dead already, so it cannot interact with its environment in any way
 * (network, executions, disks, etc).
 *
 * Usually, the functions registered in this_actor::on_exit() are in charge
 * of releasing any memory allocated by the actor during its execution.
 *
 * The other way of getting informed when an actor terminates is to connect a
 * function in the Actor::on_termination signal, that is shared between
 * all actors. Callbacks to this signal are executed for each terminating
 * actors, no matter what. This is useful in many cases, in particular
 * when developing SimGrid plugins.
 *
 * Finally, you can attach callbacks to the Actor::on_destruction signal.
 * It is also shared between all actors, and gets fired when the actors
 * are destroyed. A delay is possible between the termination of an actor
 * (ie, when it terminates executing its code) and its destruction (ie,
 * when it is not referenced anywhere in the simulation and can be collected).
 *
 * In both cases, you can stack more than one callback in the signal.
 * They will all be executed in the registration order.
 */

import org.simgrid.s4u.*;

class ActorA extends Actor {
  public void run()
  {

    // Register a lambda function to be executed once it stops
    on_exit(new CallbackBoolean() {
      @Override public void run(boolean failed)
      {
        Engine.info("I stop now");
      }
    });

    sleep_for(1);
  }
}

class ActorB extends Actor {
  public void run() { sleep_for(2); }
}
class ActorC extends Actor {
  public void run() throws SimgridException
  {
    // Register a lambda function to be executed once it stops
    on_exit(new CallbackBoolean() {
      @Override public void run(boolean failed)
      {
        if (failed) {
          Engine.info("I was killed!");
        } else
          Engine.info("Exiting gracefully.");
      }
    });

    sleep_for(3);
    Engine.info("And now, induce a deadlock by waiting for a message that will never come\n\n");
    this.get_engine().mailbox_by_name("nobody").get();
    Engine.die("Receiving is not supposed to succeed when nobody is sending");
  }
}

class actor_exiting {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]); /* - Load the platform description */

    /* Register a callback in the Actor::on_creation signal. It will be called for every started actors */
    Actor.on_creation_cb(new CallbackActor() {
      @Override public void run(Actor a)
      {
        Engine.info("Actor " + a.get_name() + " starts now.");
      }
    });
    /* Register a callback in the Actor::on_termination signal. It will be called when the actor interrupts its
     * execution (either by reaching its end or being killed) */
    Actor.on_termination_cb(new CallbackActor() {
      @Override public void run(Actor a)
      {
        Engine.info("Actor " + a.get_name() + " terminates now.");
      }
    });

    /* Create some actors */
    e.host_by_name("Tremblay").add_actor("A", new ActorA());
    e.host_by_name("Fafard").add_actor("B", new ActorB());
    e.host_by_name("Ginette").add_actor("C", new ActorC());

    e.run(); /* Run the simulation */

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
