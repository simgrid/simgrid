/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class execute_load_test extends Actor {
  public void run()
  {
    Engine e = this.get_engine();

    Host host = e.host_by_name("MyHost1");

    Engine.info(
        "Initial peak speed: %.0E flop/s; number of flops computed so far: %.0E (should be 0) and current average "
            + "load: %.5f (should be 0)",
        host.get_speed(), host.load.get_computed_flops(), host.load.get_average());

    double start = Engine.get_clock();
    Engine.info("Sleep for 10 seconds");
    sleep_for(10);

    double speed = host.get_speed();
    Engine.info(
        "Done sleeping %.2fs; peak speed: %.0E flop/s; number of flops computed so far: %.0E (nothing should have "
            + "changed)",
        Engine.get_clock() - start, host.get_speed(), host.load.get_computed_flops());

    // Run an activity
    start = Engine.get_clock();
    Engine.info("Run an activity of %.0E flops at current speed of %.0E flop/s", 200E6, host.get_speed());
    execute(200E6);

    Engine.info("Done working on my activity; this took %.2fs; current peak speed: %.0E flop/s (when I started the "
                    + "computation, the speed was set to %.0E flop/s); number of flops computed so "
                    + "far: %.2E, average load as reported by the HostLoad plugin: %.5f (should be %.5f)",
                Engine.get_clock() - start, host.get_speed(), speed, host.load.get_computed_flops(),
                host.load.get_average(),
                200E6 / (10.5 * speed * host.get_core_count() +
                         (Engine.get_clock() - start - 0.5) * host.get_speed() * host.get_core_count()));

    // ========= Change power peak =========
    int pstate = 1;
    host.set_pstate(pstate);
    Engine.info(
        "========= Requesting pstate %d (speed should be of %.0E flop/s and is of %.0E flop/s, average load is %.5f)",
        pstate, host.get_pstate_speed(pstate), host.get_speed(), host.load.get_average());

    // Run a second activity
    start = Engine.get_clock();
    Engine.info("Run an activity of %.0E flops", 100E6);
    execute(100E6);
    Engine.info(
        "Done working on my activity; this took %.2fs; current peak speed: %.0E flop/s; number of flops computed so "
            + "far: %.2E",
        Engine.get_clock() - start, host.get_speed(), host.load.get_computed_flops());

    start = Engine.get_clock();
    Engine.info("========= Requesting a reset of the computation and load counters");
    host.load.reset();
    Engine.info("After reset: %.0E flops computed; load is %.5f", host.load.get_computed_flops(),
                host.load.get_average());
    Engine.info("Sleep for 4 seconds");
    sleep_for(4);
    Engine.info("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
                Engine.get_clock() - start, host.get_speed(), host.load.get_computed_flops());

    // =========== Turn the other host off ==========
    Host host2 = e.host_by_name("MyHost2");
    Engine.info("Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 computed %.0f flops so far and has an "
                    + "average load of %.5f.",
                host2.load.get_computed_flops(), host2.load.get_average());
    host2.turn_off();
    start = Engine.get_clock();
    sleep_for(10);
    Engine.info("Done sleeping %.2f s; peak speed: %.0E flop/s; number of flops computed so far: %.0E",
                Engine.get_clock() - start, host.get_speed(), host.load.get_computed_flops());
  }
}

class change_speed extends Actor {
  public void run()
  {
    Host host = this.get_engine().host_by_name("MyHost1");
    sleep_for(10.5);
    Engine.info("I slept until now, but now I'll change the speed of this host "
                + "while the other actor is still computing! This should slow the computation down.");
    host.set_pstate(2);
  }
}

public class plugin_host_load {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    if (args.length == 1)
      Engine.die("Usage: %s platform_file\n\tExample: plugin_host_load ../platforms/energy_platform.xml\n");

    e.plugin_host_load_init();
    e.load_platform(args[0]);

    e.host_by_name("MyHost1").add_actor("load_test", new execute_load_test());
    e.host_by_name("MyHost2").add_actor("change_speed", new change_speed());

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
