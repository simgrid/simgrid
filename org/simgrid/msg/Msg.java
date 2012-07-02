/*
 * JNI interface to C code for MSG.
 * 
 * Copyright 2006,2007,2010,2011 The SimGrid Team.           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */

package org.simgrid.msg;

public final class Msg {
	/* Statically load the library which contains all native functions used in here */
	static {
		try {
			System.loadLibrary("SG_java");
		} catch(UnsatisfiedLinkError e) {
			System.err.println("Cannot load the bindings to the simgrid library: ");
			e.printStackTrace();
			System.err.println(
					"Please check your LD_LIBRARY_PATH, or copy the simgrid and SG_java libraries to the current directory");
			System.exit(1);
		}
	}
    /** Retrieve the simulation time
     * @return
     */
	public final static native double getClock();
	/**
	 * Issue a debug logging message.
	 * @param s message to log.
	 */
	public final static native void debug(String s);
	/**
	 * Issue an verbose logging message.
	 * @param s message to log.
	 */
	public final static native void verb(String s);

	/** Issue an information logging message
     * @param s
     */
	public final static native void info(String s);
	/**
	 * Issue an warning logging message.
	 * @param s message to log.
	 */
	public final static native void warn(String s);
	/**
	 * Issue an error logging message.
	 * @param s message to log.
	 */
	public final static native void error(String s);
	/**
	 * Issue an critical logging message.
	 * @param s message to log.
	 */
	public final static native void critical(String s);

	/*********************************************************************************
	 * Deployment and initialization related functions                               *
	 *********************************************************************************/

	/**
	 * The natively implemented method to initialize a MSG simulation.
	 *
	 * @param args            The arguments of the command line of the simulation.
	 *
	 * @see                    Msg.init()
	 */
	public final static native void init(String[]args);

	/**
	 * Run the MSG simulation.
	 *
	 * The simulation is not cleaned afterward (see  
	 * {@link #clean()} if you really insist on cleaning the C side), so you can freely 
	 * retrieve the informations that you want from the simulation. In particular, retrieving the status 
	 * of a process or the current date is perfectly ok. 
	 *
	 * @see                    MSG_run
	 */
	public final static native void run() ;
	
	/**
	 * Cleanup the MSG simulation.
	 * 
	 * This function is only useful if you want to chain the simulations within 
	 * the same environment. But actually, it's not sure at all that cleaning the 
	 * JVM is faster than restarting a new one, so it's probable that using this 
	 * function is not a brilliant idea. Do so at own risk.
	 *	
	 * @see                    MSG_clean
	 */
	public final static native void clean();
	

	/**
	 * The native implemented method to create the environment of the simulation.
	 *
	 * @param platformFile    The XML file which contains the description of the environment of the simulation
	 *
	 */
	public final static native void createEnvironment(String platformFile);

	/**
	 * The method to deploy the simulation.
	 *
     *
     * @param deploymentFile
     */
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
}
