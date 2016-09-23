/* Copyright (c) 2012-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.masterworker;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Worker extends Process {
  private int number;
  public Worker(Host host, int number) {
    super(host,"WRK0" + number,null);
    this.number = number;
  }

  public void main(String[] args) throws MsgException {
    Msg.info(this.getName() +" is listening on MBOX:WRK0"+ number);
    while(true) {
      Task task =null;
      try {
        task = Task.receive("MBOX:WRK0"+number);
        if (task == null)
          break;
      } catch (MsgException e) {
        Msg.debug("Received failed. I'm done. See you!");
      }
      Msg.info("Received \"" + task.getName() +  "\". Processing it.");
      task.execute();
      Msg.info(this.getName() +" executed task (" + task.getName()+")");
    }
  }
}
