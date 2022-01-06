/* Copyright (c) 2006-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package process.startkilltime;
import org.simgrid.msg.Msg;

public class Main {
  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) {
    Msg.init(args);
    if(args.length < 2) {
      Msg.info("Usage   : StartKilltime platform_file deployment_file");
      Msg.info("example : StartKilltime ../platforms/platform.xml deployment_start_kill.xml");
      System.exit(1);
    }
    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    Msg.deployApplication(args[1]);

    /*  execute the simulation. */
    Msg.run();
  }
}
