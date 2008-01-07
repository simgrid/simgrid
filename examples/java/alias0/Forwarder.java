/*
 * $Id: Forwarder.java 5059 2007-11-19 20:01:59Z mquinson $
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier         
 * All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class Forwarder extends simgrid.msg.Process {
    
	public void main(String[] args) throws JniException, NativeException 
	{
		Msg.info("hello!");
	
		int aliasCount = args.length;
		Task taskReceived;
		Task finalizeTask;
		BasicTask basicTask;
		
		int taskCount = 0;
		
		while(true) 
		{
			taskReceived = Task.receive();
				
	
			if(taskReceived instanceof FinalizeTask) 
			{
				Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.");
	
				for (int i = 0; i < aliasCount; i++) 
				{
					finalizeTask = new FinalizeTask();
					finalizeTask.send(args[i]);
				}
				
				break;
			}
			
			basicTask = (BasicTask)taskReceived;
	
			Msg.info("Received \"" + taskReceived.getName() + "\" ");
			
			Msg.info("Sending \"" + taskReceived.getName() + "\" to \"" + args[taskCount % aliasCount] + "\"");
			
			basicTask.send(args[taskCount % aliasCount]);
	
			taskCount++;
		}
	
	
		Msg.info("I'm done. See you!");
	}
}

