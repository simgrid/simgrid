/* Copyright (c) 2012-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.centralizedmutex;

import org.simgrid.msg.Msg;

class Main {
  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) {
    Msg.init(args);

    String platf  = args.length > 1 ? args[0] : "../platforms/small_platform.xml";
    String deploy =  args.length > 1 ? args[1] : "./centralizedmutex.xml";

    /* construct the platform and deploy the application */
    Msg.createEnvironment(platf);
    Msg.deployApplication(deploy);

    /*  execute the simulation. */
    Msg.run();
  }
}
