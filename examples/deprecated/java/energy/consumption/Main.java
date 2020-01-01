/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package energy.consumption;

import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;

public class Main {
  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) throws MsgException {  
    Msg.energyInit(); 
    Msg.init(args); 

    if (args.length < 1) {
      Msg.info("Usage   : Energy platform_file");
      Msg.info("Usage  : Energy ../platforms/energy_platform.xml");
      System.exit(1);
    }
    /* Construct the platform */
    Msg.createEnvironment(args[0]);
    /* Instantiate a process */
    new EnergyConsumer("MyHost1","energyConsumer").start();
    /* Execute the simulation */
    Msg.run();
    Msg.info("Total simulation time: " + Msg.getClock());
  }
}
