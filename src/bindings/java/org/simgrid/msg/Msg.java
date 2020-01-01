/* JNI interface to C code for MSG. */

/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

public final class Msg {

	/** Retrieves the simulation time */
	public static final native double getClock();
	/** Issue a debug logging message. */
	public static final native void debug(String msg);
	/** Issue a verbose logging message. */
	public static final native void verb(String msg);
	/** Issue an information logging message */
	public static final native void info(String msg);
	/** Issue a warning logging message. */
	public static final native void warn(String msg);
	/** Issue an error logging message. */
	public static final native void error(String msg);
	/** Issue a critical logging message. */
	public static final native void critical(String s);

	private Msg() {
		throw new IllegalAccessError("Utility class");
	}

	/*********************************************************************************
	 * Deployment and initialization related functions                               *
	 *********************************************************************************/

	/** Initialize a MSG simulation.
	 *
	 * @param args            The arguments of the command line of the simulation.
	 */
	public static final native void init(String[]args);
	
	/** Tell the kernel that you want to use the energy plugin */
	public static final native void energyInit();

    /** Tell the kernel that you want to use the filesystem plugin. */
	public static final native void fileSystemInit();

    /** Initializes the HostLoad plugin.
     *
	 * The HostLoad plugin provides an API to get the current load of each host.
     */
	public static final native void loadInit();

	/** Run the MSG simulation.
	 *
	 * After the simulation, you can freely retrieve the information that you want..
	 * In particular, retrieving the status of a process or the current date is perfectly ok.
	 */
	public static final native void run() ;

	/** Create the simulation environment by parsing a platform file. */
	public static final native void createEnvironment(String platformFile);

	public static final native As environmentGetRoutingRoot();

	/** Starts your processes by parsing a deployment file. */
	public static final native void deployApplication(String deploymentFile);

	/** Example launcher. You can use it or provide your own launcher, as you wish
	 * @param args
	 */
	public static void main(String[]args) {
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
