/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier         
 * All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class Master extends simgrid.msg.Process {
   public void main(String[] args) throws JniException, NativeException {
      int channel = 0;
      Msg.info("hello!");
        
      int slaveCount = 0;
      Host[] slaves = null;
      
      Msg.info("argc="+args.length);
      for (int i = 0; i<args.length; i++)	    
	Msg.info("argv:"+args[i]);
      
      if (args.length < 3) {
	 Msg.info("Master needs 3 arguments");
	 System.exit(1);
      }
      
      int numberOfTasks = Integer.valueOf(args[0]).intValue();		
      double taskComputeSize = Double.valueOf(args[1]).doubleValue();		
      double taskCommunicateSize = Double.valueOf(args[2]).doubleValue();
      BasicTask[] todo = new BasicTask[numberOfTasks];
      
      for (int i = 0; i < numberOfTasks; i++) {
	 todo[i] = new BasicTask("Task_" + i, taskComputeSize, taskCommunicateSize); 
      }
      
      slaveCount = args.length - 3;
      slaves = new Host[slaveCount];
      
      for(int i = 3; i < args.length ; i++)  {
	 try {
	    slaves[i-3] = Host.getByName(args[i]);
	 }
	 catch(HostNotFoundException e) {
	    Msg.info("Unknown host " + args[i] + ". Stopping Now!");
	    e.printStackTrace();
	    System.exit(1);
	 }
      }
      
      Msg.info("Got "+  slaveCount + " slave(s) :");
      
      for (int i = 0; i < slaveCount; i++)
	Msg.info("\t"+slaves[i].getName());
      
      Msg.info("Got "+ numberOfTasks + " task to process.");
      
      for (int i = 0; i < numberOfTasks; i++) {
	 Msg.info("Sending \"" + todo[i].getName()+ "\" to \"" + slaves[i % slaveCount].getName() + "\"");
	 
	 if((Host.currentHost()).equals(slaves[i % slaveCount])) 
	   Msg.info("Hey ! It's me ! ");
	 
	 slaves[i % slaveCount].put(channel, todo[i]);
      }
      
      Msg.info("Send completed");
      
      Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.");
      
      for (int i = 0; i < slaveCount; i++) {
	 slaves[i].put(channel, new FinalizeTask());
      }
      
      Msg.info("Goodbye now!");
    }
}
