/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example demonstrate the actor migrations.
 *
 * The worker actor first move by itself, and then start an execution.
 * During that execution, the monitor migrates the worker, that wakes up on another host.
 * The execution was of the right amount of flops to take exactly 5 seconds on the first host
 * and 5 other seconds on the second one, so it stops after 10 seconds.
 *
 * Then another migration is done by the monitor while the worker is suspended.
 *
 * Note that worker() takes an uncommon set of parameters,
 * and that this is perfectly accepted by create().
 */

import org.simgrid.s4u.*;

class Worker extends Actor {
  Host first;
  Host second;
  Worker(Host first, Host second)
  {
    this.first  = first;
    this.second = second;
  }
  public void run()
  {
    double flopAmount = first.get_speed() * 5 + second.get_speed() * 5;

    Engine.info("Let's move to %s to execute %.2f Mflops (5sec on %s and 5sec on %s)", first.get_name(),
                flopAmount / 1e6, first.get_name(), second.get_name());

    this.set_host(first);
    this.execute(flopAmount);

    Engine.info("I wake up on %s. Let's suspend a bit", this.get_host().get_name());

    this.suspend();

    Engine.info("I wake up on %s", this.get_host().get_name());
    Engine.info("Done");
  }
}

class Monitor extends Actor {
  public void run()
  {
    Engine e       = this.get_engine();
    Host boivin    = e.host_by_name("Boivin");
    Host jacquelin = e.host_by_name("Jacquelin");
    Host fafard    = e.host_by_name("Fafard");

    Actor actor = fafard.add_actor("worker", new Worker(boivin, jacquelin));

    this.sleep_for(5);

    Engine.info("After 5 seconds, move the actor to %s", jacquelin.get_name());
    actor.set_host(jacquelin);

    this.sleep_until(15);
    Engine.info("At t=15, move the actor to %s and resume it.", fafard.get_name());
    actor.set_host(fafard);
    actor.resume();
  }
}
public class actor_migrate {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    e.host_by_name("Boivin").add_actor("monitor", new Monitor());
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
