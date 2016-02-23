/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package energy;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.MsgException;

public class Energy {
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
    Host[] hosts = Host.all();
    if (hosts.length < 1) {
      Msg.info("I need at least one host in the platform file, but " + args[0] + " has no host at all");
      System.exit(42);
    }
    /* Instanciate a process */
    new EnergyConsumer(hosts[0],"energyConsumer",null).start();
    /* Execute the simulation */
    Msg.run();
  }
}
