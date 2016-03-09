/* JNI interface to C code for MSG. */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;
import org.simgrid.NativeLib;


public final class Msg {

	/** Retrieves the simulation time */
	public final static native double getClock();
	/** Issue a debug logging message. */
	public final static native void debug(String msg);
	/** Issue a verbose logging message. */
	public final static native void verb(String msg);
	/** Issue an information logging message */
	public final static native void info(String msg);
	/** Issue a warning logging message. */
	public final static native void warn(String msg);
	/** Issue an error logging message. */
	public final static native void error(String msg);
	/** Issue a critical logging message. */
	public final static native void critical(String s);

	/*********************************************************************************
	 * Deployment and initialization related functions                               *
	 *********************************************************************************/

	/** Initialize a MSG simulation.
	 *
	 * @param args            The arguments of the command line of the simulation.
	 */
	public final static native void init(String[]args);
	
	/** Tell the kernel that you want to use the energy plugin */
	public final static native void energyInit();

	/** Run the MSG simulation.
	 *
	 * The simulation is not cleaned afterward (see  
	 * {@link #clean()} if you really insist on cleaning the C side), so you can freely 
	 * retrieve the informations that you want from the simulation. In particular, retrieving the status 
	 * of a process or the current date is perfectly ok. 
	 */
	public final static native void run() ;

	/** This function is useless nowadays, just stop calling it. */
	@Deprecated
	public final static void clean(){}

	/** Create the simulation environment by parsing a platform file. */
	public final static native void createEnvironment(String platformFile);

	public final static native As environmentGetRoutingRoot();

	/** Starts your processes by parsing a deployment file. */
	public final static native void deployApplication(String deploymentFile);

	/** Example launcher. You can use it or provide your own launcher, as you wish
	 * @param args
	 * @throws MsgException
	 */
	static public void main(String[]args) throws MsgException {
		/* initialize the MSG simulation. Must be done before anything else (even logging). */
		Msg.init(args);

		if (args.length < 2) {
			Msg.info("Usage: Msg platform_file deployment_file");
			System.exit(1);
		}

		/* Load the platform and deploy the application */
		Msg.createEnvironment(args[0]);
		Msg.deployApplication(args[1]);
		/* Execute the simulation */
		Msg.run();
	}
	
	/* Class initializer, to initialize various JNI stuff */
	static {
		org.simgrid.NativeLib.nativeInit();
	}
}
