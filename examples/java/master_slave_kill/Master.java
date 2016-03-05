/* Master of a basic master/slave example in Java */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package master_slave_kill;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

import master_slave_kill.FinalizeTask;

public class Master extends Process {
  public Master(String hostname, String name) throws HostNotFoundException {
    super(hostname, name);
  }
  public void main(String[] args) throws MsgException {
    Msg.info("Master Hello!");
    Process process2 = null;
    //Create a slave on host "alice"
    try {
      Msg.info("Create process on host 'Boivin'");
      process2 = new Slave("Boivin","slave");
      process2.start();
    } catch (MsgException e){
      System.out.println("Process2!");
    }

    //Wait for slave "alice"
    while(true) {
      Task task = Task.receive("mail1");
      if (task instanceof FinalizeTask) {
        Msg.info("Received mail1!");
        break;
      }
    }
    process2.kill();

    Msg.info("Process2 is now killed, should exit now");
  }
}
