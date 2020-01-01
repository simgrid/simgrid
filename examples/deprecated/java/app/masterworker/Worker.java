/* Copyright (c) 2006-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.masterworker;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Task;
import org.simgrid.msg.TaskCancelledException;
import org.simgrid.msg.TimeoutException;
import org.simgrid.msg.TransferFailureException;
import org.simgrid.msg.Process;

public class Worker extends Process {
  public Worker(Host host, String name, String[]args) {
    super(host,name,args);
  }
  public void main(String[] args) throws TransferFailureException, HostFailureException, TimeoutException {
    if (args.length < 1) {
      Msg.info("Worker needs 1 argument (its number)");
      System.exit(1);
    }

    int num = Integer.parseInt(args[0]);
    Msg.debug("Receiving on 'worker_"+num+"'");

    while(true) {  
      Task task = Task.receive("worker_"+num);

      if ("finalize".equals(task.getName())) {
        break;
      }
      Msg.info("Received \"" + task.getName() +  "\". Processing it (my pid is "+getPID()+").");
      try {
        task.execute();
      } catch (TaskCancelledException e) {
        e.printStackTrace();
      }
    }

    Msg.info("Received Finalize. I'm done. See you!");
  }
}
