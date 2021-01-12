/* Copyright (c) 2012-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package process.migration;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Mutex;
import org.simgrid.msg.Process;

class Main {
  protected static Mutex mutex;
  protected static Process processToMigrate = null;

  private Main() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) {
    Msg.init(args);
    if(args.length < 1) {
      Msg.info("Usage   : Migration platform_file");
      Msg.info("example : Migration ../platforms/platform.xml");
      System.exit(1);
    }
    /* Create the mutex */
    mutex = new Mutex();

    /* construct the platform*/
    Msg.createEnvironment(args[0]);
    /* bypass deploymemt */
    try {
        Policeman policeman = new Policeman("Boivin","policeman");
        policeman.start();
        Emigrant emigrant   = new Emigrant("Jacquelin","emigrant");
        emigrant.start();
        processToMigrate = emigrant;
    } catch (HostNotFoundException e){
      Msg.error("Create processes failed!");
      e.printStackTrace();
    }

    /*  execute the simulation. */
    Msg.run();
  }
}
