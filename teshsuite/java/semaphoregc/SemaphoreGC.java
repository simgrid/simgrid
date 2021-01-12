/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This test ensures that the used semaphores are not garbage-collected while we still use it.
 * This was reported as bug #19893 on gforge.
 */

package semaphoregc;

import org.simgrid.msg.*;
import org.simgrid.msg.Process;

class SemCreator extends Process {
  SemCreator(Host h, String n){
    super(h, n);
  }

  public void main(String[] args) throws MsgException{
    int j;
    Msg.info("Creating 50 new Semaphores, yielding and triggering a GC after each");
    for(j = 1; j <= 50; j++) {
      new Semaphore(0);
      waitFor(10);
      System.gc();
    }
    Msg.info("It worked, we survived. The test is passed.");
  }
}

public class SemaphoreGC {
  private SemaphoreGC() {
    throw new IllegalAccessError("Utility class");
  }

  public static void main(String[] args) throws Exception {
    Msg.init(args);
    if (args.length < 1) {
      Msg.info("Usage: java -cp simgrid.jar:. semaphoregc.SemaphoreGC <deployment.xml>");
      System.exit(1);
    }
    Msg.createEnvironment(args[0]);

    new SemCreator(Host.getByName("Fafard"), "SemCreator").start();

    Msg.run();
  }
}
