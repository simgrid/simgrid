/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier         
 * Copyright 2012 The SimGrid team         
 * All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package tracing;
import org.simgrid.msg.Msg;
import org.simgrid.trace.Trace;
import org.simgrid.msg.NativeException;
 
public class TracingTest  {

   /* This only contains the launcher. If you do nothing more than than you can run 
    *   java simgrid.msg.Msg
    * which also contains such a launcher
    */
    
    public static void main(String[] args) throws NativeException {    	
		/* initialize the MSG simulation. Must be done before anything else (even logging). */
		Msg.init(args);
		if(args.length < 2) {
			Msg.info("Usage   : PingPong platform_file deployment_file");
	    	Msg.info("example : PingPong ping_pong_platform.xml ping_pong_deployment.xml");
	    	System.exit(1);
	    	}
	
		/* construct the platform and deploy the application */
		Msg.createEnvironment(args[0]);
		Msg.deployApplication(args[1]);

		/* Initialize some state for the hosts */
		Trace.hostStateDeclare ("PM_STATE"); 
		Trace.hostStateDeclareValue ("PM_STATE", "waitingPing", "0 0 1");
		Trace.hostStateDeclareValue ("PM_STATE", "sendingPong", "0 1 0");
		Trace.hostStateDeclareValue ("PM_STATE", "sendingPing", "0 1 1");
		Trace.hostStateDeclareValue ("PM_STATE", "waitingPong", "1 0 0");

		/*  execute the simulation. */
	    Msg.run();
		 Msg.clean();
    }
}
