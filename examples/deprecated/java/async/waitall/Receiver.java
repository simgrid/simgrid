/* Copyright (c) 2006-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async.waitall;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Comm;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.TimeoutException;
import org.simgrid.msg.TransferFailureException;

public class Receiver extends Process {
  public Receiver (Host host, String name) {
    super(host,name);
  }

  @Override
  public void main(String[] args) throws TransferFailureException, HostFailureException, TimeoutException {
    Comm comm = Task.irecv(getHost().getName());
    Msg.info("I started receiving on '"+ getHost().getName() +". Wait 0.1 second, and block on the communication.");
    waitFor(0.1);
    try {
    	comm.waitCompletion();
    } catch (TimeoutException e) {
    	Msg.info("Timeout while waiting for my task");
    	throw e; // Stop this process
    }
    Msg.info("I got my task, good bye.");
  }
}