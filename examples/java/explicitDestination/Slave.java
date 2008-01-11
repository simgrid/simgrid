/*
 * $Id: Slave.java 5059 2007-11-19 20:01:59Z mquinson $
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier         
 * All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class Slave extends simgrid.msg.Process 
{
	public void main(String[] args) throws JniException, NativeException 
	{
		Msg.info("Hello !");
		
		BasicTask basicTask;
		Task receivedTask;
		
		while(true) 
		{ 
			receivedTask = Task.receive(Host.currentHost().getName());	
		
			if (receivedTask instanceof FinalizeTask) 
			{
				break;
			}
			
			basicTask = (BasicTask)receivedTask;
			Msg.info("Received \"" + basicTask.getName() + "\" ");
		
			Msg.info("Processing \"" + basicTask.getName() +  "\" ");	 
			basicTask.execute();
			Msg.info("\"" + basicTask.getName() + "\" done ");
		}
		
		Msg.info("Received Finalize. I'm done. See you!");
	}
}