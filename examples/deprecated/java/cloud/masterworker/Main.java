/* Copyright (c) 2012-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.masterworker;

import java.io.File;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;

class Main {
  public static final double TASK_COMP_SIZE = 10;
  public static final double TASK_COMM_SIZE = 10;
  public static final int NHOSTS = 6;
  public static final int NSTEPS = 50;

  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) {
    Msg.init(args); 

    String platfFile = "../../examples/platforms/small_platform.xml";
    if (args.length >= 1)
    	platfFile = args[0];
    
    File f = new File(platfFile); 
    if (!f.exists()) {
      Msg.error("File " + platfFile + " does not exist in " + System.getProperty("user.dir"));
      Msg.error("Usage  : Main ../platforms/platform.xml");
    }
    
    Msg.createEnvironment(platfFile);
    Host[] hosts = Host.all();
    if (hosts.length < NHOSTS+1) {
      Msg.info("I need at least "+ (NHOSTS+1) +"  hosts in the platform file, but " + args[0] + " contains only "
               + hosts.length + " hosts");
      System.exit(42);
    }
    new Master(hosts[0],"Master",hosts).start();
    
    /* Execute the simulation */
    Msg.run();
  }
}
