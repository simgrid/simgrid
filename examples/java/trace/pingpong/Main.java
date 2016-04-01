/* Copyright (c) 2006-2007, 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package trace.pingpong;
import org.simgrid.msg.Msg;
import org.simgrid.trace.Trace;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.NativeException;

public class Main  {
  public static void main(String[] args) throws MsgException, NativeException {
    Msg.init(args);
    if(args.length < 1) {
      Msg.info("Usage   : TracingTest platform_file");
      Msg.info("example : TracingTest ../platforms/platform.xml");
      System.exit(1);
    }

    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    new Sender("Jacquelin", "Sender", new String[] {"Boivin", "Marcel"}).start();
    new Receiver ("Boivin", "Receiver", null).start();
    new Receiver ("Marcel", "Receiver", null).start();

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
