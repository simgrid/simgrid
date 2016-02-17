/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package migration;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Mutex;
import org.simgrid.msg.Process;
import org.simgrid.msg.NativeException;

public class Migration {
  public static Mutex mutex;
  public static Process processToMigrate = null;

  public static void main(String[] args) throws NativeException {      
    Msg.init(args);
    if(args.length < 2) {
      Msg.info("Usage   : Migration platform_file deployment_file");
      Msg.info("example : Migration ../platforms/platform.xml migrationDeployment.xml");
      System.exit(1);
    }
    /* Create the mutex */
    mutex = new Mutex();

    /* construct the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    Msg.deployApplication(args[1]);

    /*  execute the simulation. */
    Msg.run();
  }
}
