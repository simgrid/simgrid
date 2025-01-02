/* Copyright (c) 2010-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class executor extends Actor {
  public executor(String name, Host location) { super(name, location); }
  public void run()
  {
    /* execute() tells SimGrid to pause the calling actor
     * until its host has computed the amount of flops passed as a parameter */
    execute(98095);
    Engine.info("Done.");

    /* This simple example does not do anything beyond that */
  }
}

class privileged extends Actor {
  public privileged(String name, Host location) { super(name, location); }
  public void run()
  {
    /* This version of execute() specifies that this execution gets a larger share of the resource.
     *
     * Since the priority is 2, it computes twice as fast as a regular one.
     *
     * So instead of a half/half sharing between the two executions, we get a 1/3 vs 2/3 sharing. */
    execute(98095, 2);
    Engine.info("Done.");

    /* Note that the timings printed when running this example are a bit misleading, because the uneven sharing only
     * last until the privileged actor ends. After this point, the unprivileged one gets 100% of the CPU and finishes
     * quite quickly. */
  }
}

public class exec_basic {
  public static void main(String[] args)
  {
    var e = Engine.get_instance(args);
    if (args.length < 1)
      Engine.die("Usage: exec_basic platform_file\n\tExample: exec_basic platform.xml");

    e.load_platform(args[0]);

    new executor("executor", e.host_by_name("Tremblay"));
    new privileged("privileged", e.host_by_name("Tremblay"));

    e.run();
  }
}
