/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async;
import java.util.ArrayList;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Comm;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

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
    ArrayList<Comm> comms = new ArrayList<Comm>();

    for (int i = 0; i < tasksCount; i++) {
      Task task = new Task("Task_" + i, taskComputeSize, taskCommunicateSize); 
      Msg.info("Sending \"" + task.getName()+ "\" to \"slave_" + i % slavesCount + "\"");
      Comm comm = task.isend("slave_"+(i%slavesCount));
      comms.add(comm);
    }

    while (comms.size() > 0) {
      for (int i = 0; i < comms.size(); i++) {
        try {
          if (comms.get(i).test()) {
            comms.remove(i);
            i--;
          }
        }
        catch (Exception e) {
          e.printStackTrace();
        }
      }
      waitFor(1);
    }
    Msg.info("All tasks have been dispatched. Let's tell (asynchronously) everybody the computation is over,"+
             " and sleep 20s so that nobody gets a message from a terminated process.");

    for (int i = 0; i < slavesCount; i++) {
      FinalizeTask task = new FinalizeTask();
      task.dsend("slave_"+(i%slavesCount));
    }
    waitFor(20);

    Msg.info("Goodbye now!");
  }
}
