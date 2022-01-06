/* Copyright (c) 2014-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.migration;

import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;

public class Main {
  private static boolean endOfTest = false;

  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void setEndOfTest(){
    endOfTest=true;
  }

  public static boolean isEndOfTest(){
    return endOfTest;
  }

  public static void main(String[] args) throws MsgException {
    Msg.init(args);
    if (args.length < 1) {
      Msg.info("Usage  : Main platform_file.xml");
      System.exit(1);
    }

    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    new cloud.migration.Test("PM0","Test").start();
    Msg.run();
  }
}
