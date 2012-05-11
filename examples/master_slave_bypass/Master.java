/*
 * Master of a basic master/slave example in Java
 *
 * Copyright 2006,2007,2010 The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

package master_slave_bypass;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

import master_slave_bypass.FinalizeTask;

public class Master extends Process {
	public Master(String hostname, String name) throws HostNotFoundException {
		super(hostname, name);
	}
	public void main(String[] args) throws MsgException {
	Msg.info("Master Hello!");
	
	//Create a slave on host "alice"
	try {
			Msg.info("Create process on host 'alice'");
	        new Slave("alice","process2").start();
	    }
	catch (MsgException e){
	        	System.out.println("Process2!");
	    }
	
	//Wait for slave "alice"
	while(true)
	{  
			Task task = Task.receive("alice");
			if (task instanceof FinalizeTask) {
				Msg.info("Received Finalize. I'm done. See you!");
				break;	
			}
	}
	}
}
