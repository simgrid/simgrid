/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package io;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.MsgException;

public class IO {
  public static void main(String[] args) throws MsgException {
    Msg.init(args);
    if(args.length < 1) {
      Msg.info("Usage   : IO platform_file ");
      Msg.info("example : IO ../platforms/storage/storage.xml ");
      System.exit(1);
    }

    Msg.createEnvironment(args[0]);

    Host[] hosts = Host.all();

    Msg.info("Number of hosts:" + hosts.length);
    for (int i = 0; i < hosts.length && i < 4; i++) {
      new io.Node(hosts[i],i).start();
    }

    Msg.run();
  }
}