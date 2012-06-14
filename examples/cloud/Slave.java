/*
 * Copyright 2012. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package cloud;

import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

public class Slave extends Process {
	private int number;
	public Slave(Host host, int number) {
		super(host,"Slave " + number,null);
		this.number = number;
	}
	public void main(String[] args) throws MsgException {
		while(true) {  			
			Msg.info("Receiving on " + "slave_" + number);
			Task task = Task.receive("slave_"+number);	

			if (task instanceof FinalizeTask) {
				break;
			}
			Msg.info("Received \"" + task.getName() +  "\". Processing it.");
			try {
				task.execute();
			} catch (MsgException e) {

			}
			Msg.info("\"" + task.getName() + "\" done ");
		}

		Msg.info("Received Finalize. I'm done. See you!");
		
	}
}