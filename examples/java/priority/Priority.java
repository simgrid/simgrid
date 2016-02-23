/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package priority;
import org.simgrid.msg.Msg;
import org.simgrid.msg.NativeException;

/* Demonstrates the use of Task.setPriority to change the computation priority of a task */ 
public class Priority  {
  public static void main(String[] args) throws NativeException {
    Msg.init(args);
    if(args.length < 2) {
      Msg.info("Usage   : Priority platform_file deployment_file");
      Msg.info("example : Priority ../platforms/platform.xml priorityDeployment.xml");
      System.exit(1);
    }

    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    Msg.deployApplication(args[1]);

    /*  execute the simulation. */
    Msg.run();
  }
}
