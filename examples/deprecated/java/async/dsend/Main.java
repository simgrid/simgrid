/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async.dsend;

/** This example demonstrates the use of the Task.dsend() method.
 *
 *  This way, the sender can be detached from the communication: it is not blocked as with Task.send()
 *  and has nothing to do at the end as with Task.isend() where it must do a Comm.wait().
 */

import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;

class Main {
  private Main() {
	/* This is just to ensure that nobody creates an instance of this singleton */
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) {
    Msg.init(args);

    String platform = "../platforms/small_platform.xml";
    if (args.length >= 1) {
    	platform = args[0]; // Override the default value if passed on the command line
    }

    /* construct the platform and deploy the application */
    Msg.createEnvironment(platform);
    Host[] hosts = Host.all();
    new Sender(hosts[0],"Sender").start();
    for (int i=1; i < hosts.length; i++){
      new Receiver(hosts[i], "Receiver").start();
    }
    /*  execute the simulation. */
    Msg.run();
  }
}
