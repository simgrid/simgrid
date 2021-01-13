/* Copyright (c) 2012-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package hostload;

import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;

public class Main {
  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) {
    Msg.loadInit();
    Msg.init(args); 

    if (args.length < 1) {
      Msg.info("Usage   : Load platform_file");
      Msg.info("Usage  : Load ../platforms/small_platform.xml");
      System.exit(1);
    }
    /* Construct the platform */
    Msg.createEnvironment(args[0]);
    new LoadRunner(Host.all()[0], "").start();

    Msg.run();

  }
}
