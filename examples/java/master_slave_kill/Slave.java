/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package master_slave_kill;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Task;
import org.simgrid.msg.TimeoutException;
import org.simgrid.msg.TransferFailureException;
import org.simgrid.msg.NativeException;
import org.simgrid.msg.Process;

import master_slave_kill.FinalizeTask;

public class Slave extends Process {
  public Slave(String hostname, String name) throws HostNotFoundException {
    super(hostname, name);
  }
  public void main(String[] args) throws TransferFailureException, HostFailureException, TimeoutException, NativeException {
    Msg.info("Slave Hello!");

    FinalizeTask task = new FinalizeTask();
    Msg.info("Send Mail1!");
    task.send("mail1");

    try {
      Task.receive("mail2");
    } catch (MsgException e) {
      Msg.debug("Received failed");
      return;
    }
    Msg.info("Receive Mail2!");
  }
}
