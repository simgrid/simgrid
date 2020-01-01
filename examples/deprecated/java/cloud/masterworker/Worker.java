/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.masterworker;

import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

public class Worker extends Process {
  public Worker(Host host, String name) {
    super(host, name);
  }

  public void main(String[] args) throws MsgException {
    Msg.verb(this.getName() +" is listening on "+ getName());
    while(true) {
      Task task = null;
      try {
        task = Task.receive(getName());
      } catch (MsgException e) {
        Msg.info("Received failed. I'm done. See you!");
        exit();
      }
      Msg.verb("Received '" + task.getName() +  "'. Processing it.");
      task.execute();
      Msg.verb("Done executing task '" + task.getName() +"'");
    }
  }
}
