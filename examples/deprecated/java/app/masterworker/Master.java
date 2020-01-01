/* Master of a basic master/worker example in Java */

/* Copyright (c) 2006-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.masterworker;
import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;

public class Master extends Process {
  public Master(Host host, String name, String[]args) {
    super(host,name,args);
  } 
  public void main(String[] args) throws MsgException {
    if (args.length < 4) {
      Msg.info("Master needs 4 arguments");
      System.exit(1);
    }

    int tasksCount = Integer.parseInt(args[0]);
    double taskComputeSize = Double.parseDouble(args[1]);
    double taskCommunicateSize = Double.parseDouble(args[2]);

    int workersCount = Integer.parseInt(args[3]);

    Msg.info("Hello! My PID is "+getPID()+". Got "+  workersCount + " workers and "+tasksCount+" tasks to process");

    for (int i = 0; i < tasksCount; i++) {
      Task task = new Task("Task_" + i, taskComputeSize, taskCommunicateSize); 
      Msg.debug("Sending \"" + task.getName()+ "\" to \"worker_" + i % workersCount + "\"");
      task.send("worker_"+(i%workersCount));
    }

    Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.");

    for (int i = 0; i < workersCount; i++) {
      Task task = new Task("finalize", 0, 0);
      task.send("worker_"+(i%workersCount));
    }

    Msg.info("Goodbye now!");
  }
}
