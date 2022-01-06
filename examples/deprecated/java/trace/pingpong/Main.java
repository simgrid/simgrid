/* Copyright (c) 2006-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package trace.pingpong;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.trace.Trace;

public class Main  {
  public static final String PM_STATE = "PM_STATE";

  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) throws MsgException {
    Msg.init(args);
    if(args.length < 1) {
      Msg.info("Usage   : Main platform_file");
      Msg.info("example : Main ../platforms/platform.xml");
      System.exit(1);
    }

    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    new Sender("Jacquelin", "Sender", new String[] {"Boivin", "Tremblay"}).start();
    new Receiver ("Boivin", "Receiver", null).start();
    new Receiver ("Tremblay", "Receiver", null).start();

    /* Initialize some state for the hosts */
    Trace.hostStateDeclare (PM_STATE);
    Trace.hostStateDeclareValue (PM_STATE, "waitingPing", "0 0 1");
    Trace.hostStateDeclareValue (PM_STATE, "sendingPong", "0 1 0");
    Trace.hostStateDeclareValue (PM_STATE, "sendingPing", "0 1 1");
    Trace.hostStateDeclareValue (PM_STATE, "waitingPong", "1 0 0");

    /*  execute the simulation. */
    Msg.run();
  }
}
