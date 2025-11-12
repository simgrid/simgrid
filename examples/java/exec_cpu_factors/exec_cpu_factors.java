/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to simulate variability for CPUs, using multiplicative factors */

import org.simgrid.s4u.*;

/* *********************************************************************************************** */
class runner extends Actor {
  public void run()
  {
    double computation_amount = this.get_host().get_speed();

    Engine.info("Executing 1 tasks of %.0f flops, should take 1 second.", computation_amount);
    this.execute(computation_amount);
    Engine.info("Executing 1 tasks of %f flops, it would take .001s without factor. It'll take .002s",
                computation_amount / 10);
    this.execute(computation_amount / 1000);

    Engine.info("Finished executing. Goodbye now!");
  }
}

/*************************************************************************************************/
public class exec_cpu_factors {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    /* create platform */
    NetZone zone     = e.get_netzone_root();
    Host runner_host = zone.add_host("runner", 1e6);

    runner_host.set_cpu_factor_cb(new CallbackDHostDouble() {
      /* Variability for CPU */
      @Override public double run(Host host, double flops)
      {
        /* creates variability for tasks smaller than 1% of CPU power.
         * unrealistic, for learning purposes */
        double factor = (flops < host.get_speed() / 100) ? 0.5 : 1.0;
        Engine.info("Host %s, task with %f flops, new factor %f", host.get_name(), flops, factor);
        return factor;
      }
    });
    zone.seal();

    /* create actor runner */
    runner_host.add_actor("runner", new runner());

    /* runs the simulation */
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}