/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package sleep_host_off;

import org.simgrid.msg.*;
import org.simgrid.msg.Process;
import org.simgrid.trace.Trace;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class SleepHostOff extends Process{
  public static Host jupiter = null;

  public SleepHostOff(Host host, String name, String[]args) {
    super(host,name,args);
  }

  public void main(String[] strings) throws MsgException {

    try {
      jupiter = Host.getByName("Jupiter");
    } catch (HostNotFoundException e) {
      e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
    }

    Msg.info("**** **** **** ***** ***** Test Sleep ***** ***** **** **** ****");
    Msg.info("Test sleep: Create a process on Jupiter, the process simply make periodic sleep, turn off Jupiter");
    new Process(jupiter, "sleep", null) {
      public void main(String[] args) {
        while (true) {
          Msg.info("I'm not dead");
          try {
            Process.sleep(10);
          } catch (HostFailureException e) {
            Msg.info("catch HostException");
            e.printStackTrace();
          }
        }
      }
    }.start();

    Process.sleep(20);
    Msg.info("Stop Jupiter");
    jupiter.off();
    Msg.info("Jupiter has been stopped");
    Process.sleep(300);
    Msg.info("Test sleep seems ok, cool !(number of Process : " + Process.getCount() + ", it should be 1 (i.e. the Test one))\n");
  }
}
