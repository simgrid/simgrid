/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.pingpong;
import java.io.File;

import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;

class Main {
  protected static final int TASK_COUNT = 3;

  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) throws HostNotFoundException {
    Msg.init(args);

    String platfFile = "../../examples/platforms/small_platform.xml";
    if (args.length == 1)
    	platfFile = args[0];

    File f = new File(platfFile);
    if (!f.exists()) {
      Msg.error("File " + platfFile + " does not exist in " + System.getProperty("user.dir"));
      Msg.error("Usage  : Main ../platforms/platform.xml");
    }

    Msg.createEnvironment(platfFile);
    new Sender("Jacquelin", "Sender", new String[] {"Boivin"}).start();
    new Receiver ("Boivin", "Receiver", null).start();

    Msg.run();
  }
}
