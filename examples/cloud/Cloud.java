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

/**
 * Example showing the use of the new experimental Cloud API.
 */
public class Cloud {
	public static final double task_comp_size = 10;
	public static final double task_comm_size = 10;

	public static void main(String[] args) throws MsgException {       
	    Msg.init(args); 
	    
	    if (args.length < 1) {
	    	Msg.info("Usage	 : Cloud platform_file");
	    	Msg.info("Usage  : Cloud platform.xml");
	    	System.exit(1);
	    }
	    /* Construct the platform */
		Msg.createEnvironment(args[0]);
		  /* Retrieve the 10 first hosts of the platform file */
		Host[] hosts = Host.all();
		if (hosts.length < 10) {
			Msg.info("I need at least 10 hosts in the platform file, but " + args[0] + " contains only " + hosts.length + " hosts");
			System.exit(42);
		}
		new Master(hosts[0],"Master",hosts).start();
		/* Execute the simulation */
		Msg.run();
		
		Msg.clean();
    }
}