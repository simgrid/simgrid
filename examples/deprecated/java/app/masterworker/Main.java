/* Copyright (c) 2006-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.masterworker;

import java.io.File;

import org.simgrid.msg.Msg;

class Main {
  public static final int TASK_COMP_SIZE = 10000000;
  public static final int TASK_COMM_SIZE = 10000000;
  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) {
    /* initialize the MSG simulation. Must be done before anything else (even logging). */
    Msg.init(args);

    String platf  = args.length > 1 ? args[0] : "../platforms/small_platform.xml";
    String deploy =  args.length > 1 ? args[1] : "app/masterworker/masterworker.xml";

    Msg.verb("Platform: "+platf+"; Deployment:"+deploy+"; Current directory: "+new File(".").getAbsolutePath());

    /* construct the platform and deploy the application */
    Msg.createEnvironment(platf);
    Msg.deployApplication(deploy);
    /*  execute the simulation. */
    Msg.run();
  }
}
