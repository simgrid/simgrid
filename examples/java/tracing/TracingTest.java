/* Copyright (c) 2006-2007, 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package tracing;
import org.simgrid.msg.Msg;
import org.simgrid.trace.Trace;
import org.simgrid.msg.NativeException;

public class TracingTest  {
  public static void main(String[] args) throws NativeException {      
    Msg.init(args);
    if(args.length < 2) {
      Msg.info("Usage   : TracingTest platform_file deployment_file");
      Msg.info("example : TracingTest ../platforms/platform.xml tracingPingPongDeployment.xml");
      System.exit(1);
    }

    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    Msg.deployApplication(args[1]);

    /* Initialize some state for the hosts */
    Trace.hostStateDeclare ("PM_STATE"); 
    Trace.hostStateDeclareValue ("PM_STATE", "waitingPing", "0 0 1");
    Trace.hostStateDeclareValue ("PM_STATE", "sendingPong", "0 1 0");
    Trace.hostStateDeclareValue ("PM_STATE", "sendingPing", "0 1 1");
    Trace.hostStateDeclareValue ("PM_STATE", "waitingPong", "1 0 0");

    /*  execute the simulation. */
    Msg.run();
  }
}
