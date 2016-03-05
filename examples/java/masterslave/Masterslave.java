/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package masterslave;

import java.io.File;

import org.simgrid.msg.Msg;
import org.simgrid.msg.NativeException;

public class Masterslave {
  public static final int TASK_COMP_SIZE = 10000000;
  public static final int TASK_COMM_SIZE = 10000000;
  /* This only contains the launcher. If you do nothing more than than you can run java simgrid.msg.Msg
   * which also contains such a launcher
   */

  public static void main(String[] args) throws NativeException {
    /* initialize the MSG simulation. Must be done before anything else (even logging). */
    Msg.init(args);

    String platf  = args.length > 1 ? args[0] : "examples/java/platform.xml";
    String deploy =  args.length > 1 ? args[1] : "examples/java/masterslave/masterslaveDeployment.xml";

    Msg.verb("Platform: "+platf+"; Deployment:"+deploy+"; Current directory: "+new File(".").getAbsolutePath());

    /* construct the platform and deploy the application */
    Msg.createEnvironment(platf);
    Msg.deployApplication(deploy);
    /*  execute the simulation. */
    Msg.run();
  }
}
