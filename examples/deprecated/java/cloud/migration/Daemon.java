/* Copyright (c) 2014-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.migration;

import org.simgrid.msg.*;
import org.simgrid.msg.Process;

public class Daemon extends Process {
  private Task currentTask;
  public Daemon(VM vm) {
    super((Host)vm,"Daemon");
    currentTask = new Task(this.getHost().getName()+"-daemon-0", this.getHost().getSpeed()*100, 0);
  }
  public void main(String[] args) throws MsgException {
    int i = 1;
    while(!Main.isEndOfTest()) {
    try {
      currentTask.execute();
    } catch (HostFailureException e) {
      e.printStackTrace();
    } catch (TaskCancelledException e) {
      Msg.info("task cancelled");
      suspend(); // Suspend the process
    }
    currentTask = new Task(this.getHost().getName()+"-daemon-"+(i++), this.getHost().getSpeed()*100, 0);
  }
}

  public double getRemaining(){
    return this.currentTask.getFlopsAmount();
  }
}
