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
      
      Msg.info("Hello i'm the master");
      
      int numberoftasks = Integer.valueOf(args[0]).intValue();
      double taskComputeSize = Double.valueOf(args[1]).doubleValue();		
      double taskCommunicateSize = Double.valueOf(args[2]).doubleValue();
		
      int slavecount = args.length - 3;
      Host[] slaves = new Host[slavecount] ;
	
      for (int i = 3; i < args.length; i++) {
	 try {
	    slaves[i-3] = Host.getByName(args[i]);
	 } catch(HostNotFoundException e) {
	    Msg.info(e.toString());
	    Msg.info("Unknown host " +  args[i] +". Stopping Now! " );
	    System.exit(1);
	 }
      }
  		
      Msg.info("Got " + slavecount + " slave(s):");		
      for (int i = 0; i < slavecount; i++)
	Msg.info("\t " + slaves[i].getName());
      
      Msg.info("Got "+numberoftasks+" task(s) to process.");
      
      Channel channel = new Channel(0);
		
      for (int i = 0; i < numberoftasks; i++) {			
	 CommTimeTask task = new CommTimeTask("Task_" + i ,taskComputeSize,taskCommunicateSize);
	 task.setTime(Msg.getClock());
	 channel.put(task,slaves[i % slavecount]);
	 
//	 Msg.info("Send completed for the task " + task.getName() + " on the host " + slaves[i % slavecount].getName() +  " [" + (i % slavecount) + "]");
      }
		
      Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.");
		
      for (int i = 0; i < slavecount; i++) { 
			
	 Msg.info("Finalize host " + slaves[i].getName() +  " [" + i + "]");
	 channel.put(new FinalizeTask(),slaves[i]);
      }
      
      Msg.info("All finalize messages have been dispatched. Goodbye now!");
   }
}