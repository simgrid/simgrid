/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

import java.util.Arrays;
import java.util.Vector;

/**
 * A process may be defined as a code, with some private data, executing 
 * in a location (host). All the process used by your simulation must be
 * declared in the deployment file (XML format).
 * To create your own process you must inherit your own process from this 
 * class and override the method "main()". For example if you want to use 
 * a process named Slave proceed as it :
 *
 * (1) import the class Process of the package simgrid.msg
 * import simgrid.msg.Process;
 * 
 * public class Slave extends simgrid.msg.Process {
 *
 *  (2) Override the method function
 * 
 *  \verbatim
 * 	public void main(String[] args) {
 *		System.out.println("Hello MSG");
 *	}
 *  \endverbatim
 * }
 * The name of your process must be declared in the deployment file of your simulation.
 * For the example, for the previous process Slave this file must contains a line :
 * &lt;process host="Maxims" function="Slave"/&gt;, where Maxims is the host of the process
 * Slave. All the process of your simulation are automatically launched and managed by Msg.
 * A process use tasks to simulate communications or computations with another process. 
 * For more information see Task. For more information on host concept 
 * see Host.
 * 
 */

public abstract class Process implements Runnable {
	/**
	 * This attribute represents a bind between a java process object and
	 * a native process. Even if this attribute is public you must never
	 * access to it. It is set automatically during the build of the object.
	 */
	private long bind;
	/** Indicates if the process is started */
	boolean started;
	/**
	 * Even if this attribute is public you must never access to it.
	 * It is used to compute the id of an MSG process.
	 */
	public static long nextProcessId = 0;

	/**
	 * Even if this attribute is public you must never access to it.
	 * It is compute automatically during the creation of the object. 
	 * The native functions use this identifier to synchronize the process.
	 */
	private long id;

	/** Time at which the process should be created  */
	protected double startTime = 0;
	/** Time at which the process should be killed.
	 * 
	 * Set at creation, and used internally by SimGrid
	 */
	private double killTime = -1;

	private String name;
	
	private int pid = -1;
	private int ppid = -1;
	private Host host = null;

	/** The arguments of the method function of the process. */     
	public Vector<String> args;


	/**  Default constructor */
	protected Process() {
		this.id = nextProcessId++;
		this.name = null;
		this.bind = 0;
		this.args = new Vector<String>();
	}


	/**
	 * Constructs a new process from the name of a host and his name. The method
	 * function of the process doesn't have argument.
	 *
	 * @param hostname		The name of the host of the process to create.
	 * @param name			The name of the process.
	 *
	 * @exception			HostNotFoundException  if no host with this name exists.
	 *                      
	 *
	 */
	public Process(String hostname, String name) throws HostNotFoundException {
		this(Host.getByName(hostname), name, null);
	}
	/**
	 * Constructs a new process from the name of a host and his name. The arguments
	 * of the method function of the process are specified by the parameter args.
	 *
	 * @param hostname		The name of the host of the process to create.
	 * @param name			The name of the process.
	 * @param args			The arguments of the main function of the process.
	 *
	 * @exception			HostNotFoundException  if no host with this name exists.
	 *                      NativeException
	 * @throws NativeException
	 *
	 */ 
	public Process(String hostname, String name, String args[]) throws HostNotFoundException, NativeException {
		this(Host.getByName(hostname), name, args);
	}
	/**
	 * Constructs a new process from a host and his name. The method function of the 
	 * process doesn't have argument.
	 *
	 * @param host			The host of the process to create.
	 * @param name			The name of the process.
	 *
	 */
	public Process(Host host, String name) {
		this(host, name, null);
	}
	/**
	 * Constructs a new process from a host and his name, the arguments of here method function are
	 * specified by the parameter args.
	 *
	 * @param host			The host of the process to create.
	 * @param name			The name of the process.
	 * @param args			The arguments of main method of the process.
	 */	
	public Process(Host host, String name, String[]args) {
		this();
		this.host = host;
		if (host == null)
			throw new NullPointerException("Process name cannot be NULL");
		if (name == null)
			throw new NullPointerException("Process name cannot be NULL");
		this.name = name;

		this.args = new Vector<String>();
		if (null != args)
			this.args.addAll(Arrays.asList(args));
	}	
	/**
	 * Constructs a new process from a host and his name, the arguments of here method function are
	 * specified by the parameter args.
	 *
	 * @param host			The host of the process to create.
	 * @param name			The name of the process.
	 * @param args			The arguments of main method of the process.
	 * @param startTime		Start time of the process
	 * @param killTime		Kill time of the process
	 *
	 */
	public Process(Host host, String name, String[]args, double startTime, double killTime) {
		this();
		this.host = host;
		if (host == null)
			throw new NullPointerException("Process name cannot be NULL");
		if (name == null)
			throw new NullPointerException("Process name cannot be NULL");
		this.name = name;

		this.args = new Vector<String>();
		if (null != args)
			this.args.addAll(Arrays.asList(args));

		this.startTime = startTime;
		this.killTime = killTime;		
	}
	/**
	 * The natively implemented method to create an MSG process.
	 * @param hostName    A valid (bound) host where create the process.
	 */
	protected native void create(String hostName) throws HostNotFoundException;
	/**
	 * This method kills all running process of the simulation.
	 *
	 * @param resetPID		Should we reset the PID numbers. A negative number means no reset
	 *						and a positive number will be used to set the PID of the next newly
	 *						created process.
	 *
	 * @return				The function returns the PID of the next created process.
	 *			
	 */ 
	public static native int killAll(int resetPID);

	/** Simply kills the receiving process.
	 *
	 * SimGrid sometimes have issues when you kill processes that are currently communicating and such. We are working on it to fix the issues.
	 */
	public native void kill();
	
	/** Suspends the process. See {@link #resume()} to resume it afterward */
	public native void suspend();
	/** Resume a process that was suspended by {@link #suspend()}. */
	public native void resume();	
	/** Tests if a process is suspended.
	 *
	 * @see #suspend()
	 * @see #resume()
	 */
	public native boolean isSuspended();
	
	/**
	 * Specify whether the process should restart when its host restarts after a failure
	 * 
	 * A process naturally stops when its host stops. It starts again only if autoRestart is set to true.
	 * Otherwise, it just disappears when the host stops.
	 */
	public native void setAutoRestart(boolean autoRestart);
	/** Restarts the process from the beginning */
	public native void restart();
	/**
	 * Returns the name of the process
	 */
	public String getName() {
		return this.name;
	}
	/**
	 * Returns the host of the process.
	 * @return				The host instance of the process.
	 */ 
	public Host getHost() {
		return this.host;
	}
	/**
	 * This static method gets a process from a PID.
	 *
	 * @param PID			The process identifier of the process to get.
	 *
	 * @return				The process with the specified PID.
	 *
	 * @exception			NativeException on error in the native SimGrid code
	 */ 
	public static native Process fromPID(int PID) throws NativeException;
	/**
	 * This method returns the PID of the process.
	 *
	 * @return				The PID of the process.
	 *
	 */ 
	public int getPID()  {
		return pid;
	}
	/**
	 * This method returns the PID of the parent of a process.
	 *
	 * @return				The PID of the parent of the process.
	 *
	 */ 
	public int getPPID()  {
		return ppid;
	}
	/**
	 * Returns the value of a given process property. 
	 */
	public native String getProperty(String name);

	/**
	 * Set the kill time of the process
	 * @param killTime the time when the process is killed
	 */
	public native void setKillTime(double killTime);

	/**
	 * This static method returns the currently running process.
	 *
	 * @return				The current process.
	 *
	 */ 
	public static native Process getCurrentProcess();
	/**
	 * Migrates a process to another host.
	 *
	 * @param host			The host where to migrate the process.
	 *
	 */
	public native void migrate(Host host);	
	/**
	 * Makes the current process sleep until millis millisecondes have elapsed.
	 * You should note that unlike "waitFor" which takes seconds, this method takes milliseconds.
	 * FIXME: Not optimal, maybe we should have two native functions.
	 * @param millis the length of time to sleep in milliseconds.
	 */
	public static void sleep(long millis) throws HostFailureException  {
		sleep(millis,0);
	}
	/**
	 * Makes the current process sleep until millis milliseconds and nanos nanoseconds 
	 * have elapsed.
	 * Unlike {@link #waitFor(double)} which takes seconds, this method takes 
	 * milliseconds and nanoseconds.
	 * Overloads Thread.sleep.
	 * @param millis the length of time to sleep in milliseconds.
	 * @param nanos additionnal nanoseconds to sleep.
	 */
	public native static void sleep(long millis, int nanos) throws HostFailureException;
	/**
	 * Makes the current process sleep until time seconds have elapsed.
	 * @param seconds		The time the current process must sleep.
	 */ 
	public native void waitFor(double seconds) throws HostFailureException;    
	/**
	 * This method actually creates and run the process.
	 * It is a noop if the process is already launched.
	 * @throws HostNotFoundException
	 */
	public final void start() throws HostNotFoundException {
		if (!started) {
			started = true;
			create(host.getName());
		}
	}

	/** This method runs the process. Il calls the method function that you must overwrite. */
	@Override
	public void run() {

		String[] args = null;      /* do not fill it before the signal or this.args will be empty */

		try {
			args = new String[this.args.size()];
			if (this.args.size() > 0) {
				this.args.toArray(args);
			}

			this.main(args);
		} catch(MsgException e) {
			e.printStackTrace();
			Msg.info("Unexpected behavior. Stopping now");
			System.exit(1);
		}
		catch(ProcessKilledError pk) {
		}	
		exit();
	}

	/**
	 * The main function of the process (to implement).
	 *
	 * @param args
	 * @throws MsgException
	 */
	public abstract void main(String[]args) throws MsgException;

	public native void exit();    
	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	private static native void nativeInit();
	static {
		org.simgrid.NativeLib.nativeInit();
		nativeInit();
	}
	/**
	 * This static method returns the current amount of processes running
	 *
	 * @return			The count of the running processes
	 */ 
	public native static int getCount();

}
