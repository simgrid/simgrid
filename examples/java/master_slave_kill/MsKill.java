/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package master_slave_kill;

import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.NativeException;

public class MsKill {
  /* This only contains the launcher. If you do nothing more than than you can run  java simgrid.msg.Msg
   * which also contains such a launcher
   */

  public static void main(String[] args) throws NativeException {
    /* initialize the MSG simulation. Must be done before anything else (even logging). */
    Msg.init(args);
    Msg.createEnvironment(args[0]);

    /* bypass deploymemt */
    try {
      Master process1 = new Master("Jacquelin","master");
      process1.start();
    } catch (MsgException e){
      System.out.println("Create processes failed!");
    }

    /*  execute the simulation. */
    Msg.run();
  }
}
