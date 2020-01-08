/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package energy.vm;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;

class Main {
  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) {
    Msg.energyInit();
    Msg.init(args);

    if (args.length < 1) {
      Msg.info("Usage: Main ../platforms/energy_platform_file.xml");
      System.exit(1);
    }

    /* construct the platform */
    Msg.createEnvironment(args[0]);

    /* Create and start a runner for the experiment */
    new EnergyVMRunner(Host.all()[0],"energy VM runner",null).start();

    Msg.run();
  }
}
