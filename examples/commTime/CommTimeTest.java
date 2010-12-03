/*
 * Copyright 2006,2007,2010 The SimGrid Team. All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

package commTime;
import org.simgrid.msg.Msg;
import org.simgrid.msg.NativeException;

public class CommTimeTest {
	
   /* This only contains the launcher. If you do nothing more than than you can run 
    *   java simgrid.msg.Msg
    * which also contains such a launcher
    */
   
    public static void main(String[] args) throws NativeException {
    	
	/* initialize the MSG simulation. Must be done before anything else (even logging). */
	Msg.init(args);

       if(args.length < 2) {    		
	  Msg.info("Usage   : CommTime platform_file deployment_file");
	  Msg.info("example : CommTime comm_time_platform.xml comm_time_deployment.xml");
	  System.exit(1);
    	}
    	
	/* construct the platform and deploy the application */
	Msg.createEnvironment(args[0]);
	Msg.deployApplication(args[1]);
		
	/*  execute the simulation. */
        Msg.run();
    }
}
