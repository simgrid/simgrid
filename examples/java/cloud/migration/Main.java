/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.migration;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.NativeException;

public class Main {
  private static boolean endOfTest = false;

  public static void setEndOfTest(){
    endOfTest=true;
  }

  public static boolean isEndOfTest(){
    return endOfTest;
  }

  public static void main(String[] args) throws NativeException {
    Msg.init(args);

    if (args.length < 2) {
      Msg.info("Usage  : Main platform_file.xml dployment_file.xml");
      System.exit(1);
    }

    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    Msg.deployApplication(args[1]);

    Msg.run();
  }
}
