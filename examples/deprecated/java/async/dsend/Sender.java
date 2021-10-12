/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async.dsend;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Sender extends Process {
  public Sender (Host host, String name){
    super(host,name);
  }

  @Override
  public void main(String[] args) throws MsgException {
    double taskComputeSize =0;
    double taskCommunicateSize = 5000000;
    Host[] hosts = Host.all();
    int receiverCount = hosts.length - 1;

    Msg.info("Hello! Got "+ receiverCount + " receivers to contact");

    for (int i = 1; i <= receiverCount; i++) {
      Task task = new Task("Task_" + i, taskComputeSize, taskCommunicateSize);
      Msg.info("Sending \"" + task.getName()+ "\" to \"" + hosts[i].getName() + "\"");
      task.dsend(hosts[i].getName());
    }

    Msg.info("All tasks have been (asynchronously) dispatched."+
             " Let's sleep for 20s so that nobody gets a message from a terminated process.");

    waitFor(20);

    Msg.info("Done sleeping. Goodbye now!");
  }
}
