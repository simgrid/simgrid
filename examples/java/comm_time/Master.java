/*
 * Master of a basic master/slave example in Java
 *
 * Copyright 2006,2007,2010 The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class Master extends simgrid.msg.Process {
   public void main(String[] args) throws JniException, NativeException {
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
