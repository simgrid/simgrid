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

public class Master extends Process {
	public Master(String hostname, String name) throws HostNotFoundException {
		super(hostname, name);
	}
	public void main(String[] args) throws MsgException {
		Msg.info("Master Hello!");
/*	try {
	        Slave process2 = new Slave("alice","process2");
	    }
	catch (MsgException e){
	        	System.out.println("Mes couilles!!!!!");
	    } */
	}
}
