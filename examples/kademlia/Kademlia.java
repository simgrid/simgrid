/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
package kademlia;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
/**
 * Main class of the simulation. Launch the simulation.
 */
public class Kademlia {
	public static void main(String[] args) throws MsgException {
		/* initialize the MSG simulation. Must be done before anything else (even logging). */
		Msg.init(args);
	    	if(args.length < 2) {
	    		Msg.info("Usage   : Kademlia platform_file deployment_file");
	        	Msg.info("example : Kademlia platform.xml deployment.xml");
	        	System.exit(1);
	    	}
		
		/* construct the platform and deploy the application */
		Msg.createEnvironment(args[0]);
		Msg.deployApplication(args[1]);
			
		/*  execute the simulation. */
        Msg.run();		
	}
}
