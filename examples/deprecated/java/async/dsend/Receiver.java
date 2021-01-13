/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async.dsend;
import org.simgrid.msg.Msg;
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
    Msg.info("Receiving on '"+ getHost().getName() + "'");
    Task.receive(getHost().getName());
    Msg.info("Received a task. I'm done. See you!");
  }
}