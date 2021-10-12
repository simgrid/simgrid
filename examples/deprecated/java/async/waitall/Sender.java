/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async.waitall;

import org.simgrid.msg.Comm;
import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

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

    Msg.info("I have "+ receiverCount + " receivers to contact");

    Comm[] communicators = new Comm[receiverCount];
    for (int i = 1; i <= receiverCount; i++) {
      Task task = new Task("Task_" + i, taskComputeSize, taskCommunicateSize);
      Msg.info("Start the Sending '" + task.getName()+ "' to '" + hosts[i].getName() + "'");
      communicators[i-1] = task.isend(hosts[i].getName());
    }

    Msg.info("All tasks have been (asynchronously) dispatched. Let's wait for their completion.");
    Comm.waitAll(communicators);

    Msg.info("Goodbye now!");
  }
}
