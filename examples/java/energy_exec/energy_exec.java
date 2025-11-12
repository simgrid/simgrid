/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class dvfs extends Actor {
  public void run()
  {
    var e      = get_engine();
    Host host1 = e.host_by_name("MyHost1");
    Host host2 = e.host_by_name("MyHost2");

    Engine.info("Energetic profile: %s", host1.get_property("wattage_per_state"));
    Engine.info("Initial peak speed=%.0E flop/s; Energy dissipated =%.0E J", host1.get_speed(),
                host1.get_consumed_energy());

    double start = Engine.get_clock();
    Engine.info("Sleep for 10 seconds");
    sleep_for(10);
    Engine.info("Done sleeping (duration: %.2f s). Current peak speed=%.0E; Energy dissipated=%.2f J",
                Engine.get_clock() - start, host1.get_speed(), host1.get_consumed_energy());

    // Execute something
    start             = Engine.get_clock();
    double flopAmount = 100E6;
    Engine.info("Run a computation of %.0E flops", flopAmount);
    execute(flopAmount);
    Engine.info(
        "Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; Current consumption: from %.0fW to %.0fW"
            + " depending on load; Energy dissipated=%.0f J",
        Engine.get_clock() - start, host1.get_speed(), host1.get_wattmin_at(host1.get_pstate()),
        host1.get_wattmax_at(host1.get_pstate()), host1.get_consumed_energy());

    // ========= Change power peak =========
    int pstate = 2;
    host1.set_pstate(pstate);
    Engine.info("========= Requesting pstate %d (speed should be of %.0E flop/s and is of %.0E flop/s)", pstate,
                host1.get_pstate_speed(pstate), host1.get_speed());

    // Run another computation
    start = Engine.get_clock();
    Engine.info("Run a computation of %.0E flops", flopAmount);
    execute(flopAmount);
    Engine.info("Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
                Engine.get_clock() - start, host1.get_speed(), host1.get_consumed_energy());

    start = Engine.get_clock();
    Engine.info("Sleep for 4 seconds");
    sleep_for(4);
    Engine.info("Done sleeping (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
                Engine.get_clock() - start, host1.get_speed(), host1.get_consumed_energy());

    // =========== Turn the other host off ==========
    Engine.info("Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 dissipated %.0f J so far.",
                host2.get_consumed_energy());
    host2.turn_off();
    start = Engine.get_clock();
    sleep_for(10);
    Engine.info("Done sleeping (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
                Engine.get_clock() - start, host1.get_speed(), host1.get_consumed_energy());
  }
}

public class energy_exec {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.plugin_host_energy_init();

    if (args.length < 1)
      Engine.die("Usage: java -cp energy_exec.jar energy_exec platform_file\n\tExample: java -cp energy_exec.jar "
                 + "energy_exec ../platforms/energy_platform.xml\n");

    e.load_platform(args[0]);
    e.host_by_name("MyHost1").add_actor("dvfs_test", new dvfs());

    e.run();

    Engine.info("End of simulation.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
