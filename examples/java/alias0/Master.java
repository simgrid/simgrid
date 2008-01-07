/*
 * $Id: Master.java 5059 2007-11-19 20:01:59Z mquinson $
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier         
 * All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class Master extends simgrid.msg.Process 
{
   public void main(String[] args) throws JniException, NativeException 
   {
	
		Msg.info("hello!");
		
		
		Msg.info("argc="+args.length);
		
		for (int i = 0; i<args.length; i++)	    
			Msg.info("argv:"+args[i]);
		
		if (args.length < 3) 
		{
			Msg.info("Master needs 3 arguments");
			System.exit(1);
		}
		
		int taskCount = Integer.valueOf(args[0]).intValue();	
			
		double taskComputeSize = Double.valueOf(args[1]).doubleValue();
				
		double taskCommunicateSize = Double.valueOf(args[2]).doubleValue();
		
		BasicTask[] basicTasks = new BasicTask[taskCount];
		
		for (int i = 0; i < taskCount; i++) 
		{
			basicTasks[i] = new BasicTask("Task_" + i, taskComputeSize, taskCommunicateSize); 
		}
		
		int aliasCount = args.length - 3;
		
		String[] aliases = new String[aliasCount];
		
		for(int i = 3; i < args.length ; i++) 
			aliases[i - 3] = args[i];
		
		Msg.info("Got "+  aliasCount + " alias(es) :");
		
		for (int i = 0; i < aliasCount; i++)
			Msg.info("\t"+ aliases[i]);
		
		Msg.info("Got "+ taskCount + " task to process.");
		
		for (int i = 0; i < taskCount; i++) 
		{	
			Msg.info("current host name : " + (aliases[i % aliasCount].split(":"))[0]);
			
			if((Host.currentHost().getName()).equals((aliases[i % aliasCount].split(":"))[0]))
				Msg.info("Hey ! It's me ! ");
				
			basicTasks[i].send(aliases[i % aliasCount]);
		}
		
		Msg.info("Send completed");
		
		Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.");
		
		FinalizeTask finalizeTask;
		
		for (int i = 0; i < aliasCount; i++) 
		{
			finalizeTask = new FinalizeTask();
			finalizeTask.send(aliases[i]);
			
		}
		
		Msg.info("Goodbye now!");
	}
}
