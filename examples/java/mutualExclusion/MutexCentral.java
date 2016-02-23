/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package mutualExclusion;

import org.simgrid.msg.Msg;
import org.simgrid.msg.NativeException;

public class MutexCentral {
  public static void main(String[] args) throws NativeException {
    Msg.init(args);

    if(args.length < 2) {
      Msg.info("Usage: MutexCentral platform_file deployment_file");
      Msg.info("Fallback to default values");
      Msg.createEnvironment("../platform/small_platform.xml");
      Msg.deployApplication("mutex_centralized_deployment.xml");
    } else {
      /* construct the platform and deploy the application */
      Msg.createEnvironment(args[0]);
      Msg.deployApplication(args[1]);
    }

    /*  execute the simulation. */
    Msg.run();
  }
}
