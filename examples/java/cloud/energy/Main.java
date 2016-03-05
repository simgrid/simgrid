/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.energy;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.NativeException;

public class Main {

  public static void main(String[] args) throws NativeException, HostNotFoundException {
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
