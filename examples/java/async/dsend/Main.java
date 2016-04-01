/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async.dsend;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.NativeException;
import org.simgrid.msg.HostNotFoundException;

public class Main {
  public static void main(String[] args) throws NativeException, HostNotFoundException {
    Msg.init(args);

    if (args.length < 1) {
    Msg.info("Usage   : Main platform_file");
    Msg.info("example : Main ../platforms/small_platform.xml");
    System.exit(1);
  }

    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    Host[] hosts = Host.all();
    new Sender(hosts[0],"Sender").start();
    for (int i=1; i < hosts.length; i++){
      new Receiver(hosts[i], "Receiver").start();
    }
    /*  execute the simulation. */
    Msg.run();
  }
}
