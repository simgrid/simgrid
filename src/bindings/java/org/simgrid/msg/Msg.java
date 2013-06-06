/*
 * JNI interface to C code for MSG.
 * 
 * Copyright 2006-2012 The SimGrid Team.           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */

package org.simgrid.msg;

import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.File;


public final class Msg {
	/* Statically load the library which contains all native functions used in here */
	static private boolean isNativeInited = false;
	public static void nativeInit() {
		if (isNativeInited)
			return;
		try {
			/* prefer the version on disk, if existing */
			System.loadLibrary("SG_java");
		} catch (UnsatisfiedLinkError e) {
			/* If not found, unpack the one bundled into the jar file and use it */
			loadLib("simgrid");
			loadLib("SG_java");
		}
		isNativeInited = true;
	}
	static {
		nativeInit();
	}
	private static void loadLib (String name) {
	  String Os = System.getProperty("os.name");
	  //Windows may report its name in java differently from cmake, which generated the path
		if(Os.toLowerCase().indexOf("win") >= 0) Os = "Windows";
		String Path = "NATIVE/"+Os+"/"+System.getProperty("os.arch")+"/";

		String filename=name;
		InputStream in = Msg.class.getClassLoader().getResourceAsStream(Path+filename);
		
		if (in == null) {
			filename = "lib"+name+".so";
			in = Msg.class.getClassLoader().getResourceAsStream(Path+filename);
		} 
		if (in == null) {
			filename = name+".dll";
			in = Msg.class.getClassLoader().getResourceAsStream(Path+filename);
		}  
		if (in == null) {
			filename = "lib"+name+".dll";
			in = Msg.class.getClassLoader().getResourceAsStream(Path+filename);
		}  
		if (in == null) {
			filename = "lib"+name+".dylib";
			in = Msg.class.getClassLoader().getResourceAsStream(Path+filename);
		}  
		if (in == null) {
			throw new RuntimeException("Cannot find library "+name+" in path "+Path+". Sorry, but this jar does not seem to be usable on your machine.");
		}
// Caching the file on disk: desactivated because it could fool the users 		
//		if (new File(filename).isFile()) {
//			// file was already unpacked -- use it directly
//			System.load(new File(".").getAbsolutePath()+File.separator+filename);
//			return;
//		}
		try {
			// We must write the lib onto the disk before loading it -- stupid operating systems
			File fileOut = new File(filename);
//			if (!new File(".").canWrite()) {
//				System.out.println("Cannot write in ."+File.separator+filename+"; unpacking the library into a temporary file instead");
				fileOut = File.createTempFile("simgrid-", ".tmp");
				// don't leak the file on disk, but remove it on JVM shutdown
				Runtime.getRuntime().addShutdownHook(new Thread(new FileCleaner(fileOut.getAbsolutePath())));
//			}
//			System.out.println("Unpacking SimGrid native library to " + fileOut.getAbsolutePath());
			OutputStream out = new FileOutputStream(fileOut);
			
			/* copy the library in position */  
			byte[] buffer = new byte[4096]; 
			int bytes_read; 
			while ((bytes_read = in.read(buffer)) != -1)     // Read until EOF
				out.write(buffer, 0, bytes_read); 
		      
			/* close all file descriptors, and load that shit */
			in.close();
			out.close();
			System.load(fileOut.getAbsolutePath());
		} catch (Exception e) {
			System.err.println("Cannot load the bindings to the simgrid library: ");
			e.printStackTrace();
			System.err.println("This jar file does not seem to fit your system, sorry");
			System.exit(1);
		}
	}		
	/* A hackish mechanism used to remove the file containing our library when the JVM shuts down */
	private static class FileCleaner implements Runnable {
		private String target;
		public FileCleaner(String name) {
			target = name;
		}
        public void run() {
            try {
                new File(target).delete();
            } catch(Exception e) {
                System.out.println("Unable to clean temporary file "+target+" during shutdown.");
                e.printStackTrace();
            }
        }    
	}

    /** Retrieve the simulation time
     * @return The simulation time.
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
	 */
	public final static native void init(String[]args);

	/**
	 * Run the MSG simulation.
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

	/**
	 * The native implemented method to create the environment of the simulation.
	 *
	 * @param platformFile    The XML file which contains the description of the environment of the simulation
	 *
	 */
	public final static native void createEnvironment(String platformFile);

	public final static native As environmentGetRoutingRoot();

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
