/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async.waitall;

/** This example demonstrates the use of the asynchronous communications
 *
 *  Task.isend() and Task.irecv() are used to start the communications in non-blocking mode.
 *
 *  The sends are then blocked onto with Comm.waitCompletion(), that locks until the given
 *  communication terminates.
 *
 *  The receives are packed into an array, and the sender blocks until all of them terminate
 *  with Comm.waitAll().
 */


import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;

class Main {
  private Main() {
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
