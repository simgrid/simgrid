/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package process.kill;

import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;

public class Main {
  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) {
    /* initialize the MSG simulation. Must be done before anything else (even logging). */
    Msg.init(args);
    Msg.createEnvironment(args[0]);

    /* bypass deploymemt */
    try {
      Killer killer = new Killer("Jacquelin","killer");
      killer.start();
    } catch (MsgException e){
      Msg.error("Create processes failed!");
    }

    /*  execute the simulation. */
    Msg.run();
  }
}
