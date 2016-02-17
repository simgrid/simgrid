/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package commTime;

import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

public class Master extends Process {
  public Master(Host host, String name, String[]args) {
    super(host,name,args);
  }

  public void main(String[] args) throws MsgException {
    if (args.length < 4) {
      Msg.info("Master needs 4 arguments");
      System.exit(1);
    }

    int tasksCount = Integer.valueOf(args[0]).intValue();    
    double taskComputeSize = Double.valueOf(args[1]).doubleValue();    
    double taskCommunicateSize = Double.valueOf(args[2]).doubleValue();

    int slavesCount = Integer.valueOf(args[3]).intValue();

    Msg.info("Hello! Got "+  slavesCount + " slaves and "+tasksCount+" tasks to process");

    for (int i = 0; i < tasksCount; i++) {
      Task task = new Task("Task_" + i, taskComputeSize, taskCommunicateSize);
      if (i%1000==0)
         Msg.info("Sending \"" + task.getName()+ "\" to \"slave_" + i % slavesCount + "\"");
         task.send("slave_"+(i%slavesCount));
       }

    Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.");

    for (int i = 0; i < slavesCount; i++) {
      FinalizeTask task = new FinalizeTask();
      task.send("slave_"+(i%slavesCount));
    }
    Msg.info("Goodbye now!");
  }
}
