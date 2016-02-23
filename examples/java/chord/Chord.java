/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package chord;

import org.simgrid.msg.Msg;

public class Chord {
  public static void main(String[] args) {
    Msg.init(args);
    if(args.length < 2) {
      Msg.info("Usage   : Chord platform_file deployment_file");
      Msg.info("example : Chord ../platforms/platform.xml chord.xml");
      System.exit(1);
    }
    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    Msg.deployApplication(args[1]);

    /*  execute the simulation. */
    Msg.run();
  }
}
