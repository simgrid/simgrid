/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.masterworker;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.MsgException;

class Main {
  public static final double TASK_COMP_SIZE = 10;
  public static final double TASK_COMM_SIZE = 10;
  public static final int NHOSTS = 2 ; 

  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) throws MsgException {
    Msg.init(args); 

    if (args.length < 1) {
      Msg.info("Usage   : Main platform_file");
      Msg.info("Usage  : Main ../platforms/platform.xml");
      System.exit(1);
    }

    /* Construct the platform */
    Msg.createEnvironment(args[0]);
    Host[] hosts = Host.all();
    if (hosts.length < NHOSTS+1) {
      Msg.info("I need at least "+ (NHOSTS+1) +"  hosts in the platform file, but " + args[0] + " contains only "
               + hosts.length + " hosts");
      System.exit(42);
    }
    Msg.info("Start "+ NHOSTS +" hosts");
    new Master(hosts[0],"Master",hosts).start();
    /* Execute the simulation */
    Msg.run();
  }
}
