/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package SleepHostOff;

import org.simgrid.msg.*;
import org.simgrid.msg.Process;

class Sleeper extends Process {
  public Sleeper(Host host, String name, String[] args) {
    super(host,name,args);
  }
  public void main(String[] args) {
    while (true) {
      Msg.info("I'm not dead");
      try {
        Process.sleep(10);
      } catch (HostFailureException e) {
        Msg.info("catch HostException: "+e.getLocalizedMessage());
        break; //Break is needed to finalize the endless loop 
      }
    }
  }
}

class TestRunner extends Process {
  public TestRunner(Host host, String name, String[] args) {
    super(host,name,args);
  }

  public void main(String[] strings) throws MsgException {
    Host host = Host.all()[1];

    Msg.info("**** **** **** ***** ***** Test Sleep ***** ***** **** **** ****");
    Msg.info("Test sleep: Create a process on "+host.getName()+" that simply make periodic sleep, turn off "
             +host.getName());
    new Sleeper(host, "Sleeper", null).start();

    waitFor(0.02);
    Msg.info("Stop "+host.getName());
    host.off();
    Msg.info(host.getName()+" has been stopped");
    waitFor(0.3);
    Msg.info("Test sleep seems ok, cool! (number of Process : " + Process.getCount() 
             + ", it should be 1 (i.e. the Test one))");
  }
}

public class SleepHostOff {
  public static void main(String[] args) throws Exception {
    Msg.init(args);

    if (args.length < 1) {
      Msg.info("Usage: java -cp simgrid.jar:. sleep_host_off.SleepHostOff <platform.xml>");
      System.exit(1);
    }

    Msg.createEnvironment(args[0]);

    Host[] hosts = Host.all();
    new TestRunner(hosts[0], "TestRunner", null).start();

    Msg.run();
  }
}
