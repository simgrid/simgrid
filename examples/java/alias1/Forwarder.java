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

public class Forwarder extends simgrid.msg.Process 
{
    
	public void main(String[] args) throws JniException, NativeException 
	{
		Msg.info("hello!");
		
		int aliasCount = args.length;
		String[] aliases = new String[aliasCount];
		
		for (int i = 0; i < args.length; i++) 
		{	      
			aliases[i] = args[i];
		}
		
		int taskCount = 0;
		
		Task receivedTask;
		FinalizeTask finalizeTask;
		BasicTask task;
		
		while(true) 
		{
			receivedTask = Task.receive(Host.currentHost().getName());	
			
			if(receivedTask instanceof FinalizeTask) 
			{
				Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.");
		
				for (int cpt = 0; cpt < aliasCount; cpt++) 
				{
					finalizeTask = new FinalizeTask();
					finalizeTask.send(aliases[cpt]);
				}
				
				break;
			}
		
			task = (BasicTask)receivedTask;
			
			Msg.info("Received \"" + task.getName() + "\" ");
			
			Msg.info("Sending \"" + task.getName() + "\" to \"" + aliases[taskCount % aliasCount] + "\"");
			
			task.send(aliases[taskCount % aliasCount]);
			
			taskCount++;
		}
		
		
		Msg.info("I'm done. See you!");
	}
}

