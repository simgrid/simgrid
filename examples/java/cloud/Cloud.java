/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.MsgException;

public class Cloud {
  public static final double task_comp_size = 10;
  public static final double task_comm_size = 10;
  public static final int hostNB = 2 ; 
  public static void main(String[] args) throws MsgException {
    Msg.init(args); 

    if (args.length < 1) {
      Msg.info("Usage   : Cloud platform_file");
      Msg.info("Usage  : Cloud ../platforms/platform.xml");
      System.exit(1);
    }

    /* Construct the platform */
    Msg.createEnvironment(args[0]);
    Host[] hosts = Host.all();
    if (hosts.length < hostNB+1) {
      Msg.info("I need at least "+ (hostNB+1) +"  hosts in the platform file, but " + args[0] + " contains only "
               + hosts.length + " hosts");
      System.exit(42);
    }
    Msg.info("Start"+ hostNB +"  hosts");
    new Master(hosts[0],"Master",hosts).start();
    /* Execute the simulation */
    Msg.run();
  }
}
