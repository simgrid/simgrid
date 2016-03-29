/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package energy.consumption;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.MsgException;

public class Main {
  public static final double task_comp_size = 10;
  public static final double task_comm_size = 10;
  public static final int hostNB = 2 ; 

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
    /* Instanciate a process */
    new EnergyConsumer("MyHost1","energyConsumer").start();
    /* Execute the simulation */
    Msg.run();
    Msg.info("Total simulation time: "+  Msg.getClock());
  }
}
