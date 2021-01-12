/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * A process may be defined as a code, with some private data, executing
 * in a location (host). All the process used by your simulation must be
 * declared in the deployment file (XML format).
 * To create your own process you must inherit your own process from this
 * class and override the method "main()". For example if you want to use
 * a process named Worker proceed as it :
 *
 * (1) import the class Process of the package simgrid.msg
 * import simgrid.msg.Process;
 *
 * public class Worker extends simgrid.msg.Process {
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
 * For the example, for the previous process Worker this file must contains a line :
 * &lt;process host="Maxims" function="Worker"/&gt;, where Maxims is the host of the process
 * Worker. All the process of your simulation are automatically launched and managed by Msg.
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
	private long bind = 0;
	/** Indicates if the process is started */

	/** Time at which the process should be created  */
	protected double startTime = 0;
	/** Time at which the process should be killed */
	private double killTime = -1; // Used from the C world

	private String name = null;
	
	private int pid = -1;
	private int ppid = -1;
	private Host host = null;

	/** The arguments of the method function of the process. */
	private ArrayList<String> args = new ArrayList<>();

	/**
	 * Constructs a new process from the name of a host and his name. The method
	 * function of the process doesn't have argument.
	 *
	 * @param hostname		Where to create the process.
	 * @param name			The name of the process.
	 *
	 * @exception			HostNotFoundException  if no host with this name exists.
	 *
	 *
	 */
	protected Process(String hostname, String name) throws HostNotFoundException {
		this(Host.getByName(hostname), name, null);
	}
	/**
	 * Constructs a new process from the name of a host and his name. The arguments
	 * of the method function of the process are specified by the parameter args.
	 *
	 * @param hostname		Where to create the process.
	 * @param name			The name of the process.
	 * @param args			The arguments of the main function of the process.
	 *
	 * @exception			HostNotFoundException  if no host with this name exists.
	 *
	 */
	protected Process(String hostname, String name, String[] args) throws HostNotFoundException {
		this(Host.getByName(hostname), name, args);
	}
	/**
	 * Constructs a new process from a host and his name. The method function of the
	 * process doesn't have argument.
	 *
	 * @param host			Where to create the process.
	 * @param name			The name of the process.
	 *
	 */
	protected Process(Host host, String name) {
		this(host, name, null);
	}
	/**
	 * Constructs a new process from a host and his name, the arguments of here method function are
	 * specified by the parameter args.
	 *
	 * @param host			Where to create the process.
	 * @param name			The name of the process.
	 * @param argsParam		The arguments of main method of the process.
	 */	
	protected Process(Host host, String name, String[]argsParam)
	{
		if (host == null)
			throw new IllegalArgumentException("Cannot create a process on the null host");
		if (name == null)
			throw new IllegalArgumentException("Process name cannot be null");
		
		this.host = host;
		this.name = name;

		this.args = new ArrayList<>();
		if (null != argsParam)
			this.args.addAll(Arrays.asList(argsParam));
	}
	/**
	 * Constructs a new process from a host and his name, the arguments of here method function are
	 * specified by the parameter args.
	 *
	 * @param host			Where to create the process.
	 * @param name			The name of the process.
	 * @param args			The arguments of main method of the process.
	 * @param startTime		Start time of the process
	 * @param killTime		Kill time of the process
	 *
	 */
	protected Process(Host host, String name, String[]args, double startTime, double killTime) {
		this(host, name, args);
		this.startTime = startTime;
		this.killTime = killTime;
	}
	/**
	 * The native method to create an MSG process.
	 * @param host    where to create the process.
	 */
	protected native void create(Host host);
	
	/**
	 * This method kills all running process of the simulation.
	 */
	public static native void killAll();

	/** Simply kills the receiving process.
	 *
	 * SimGrid sometimes have issues when you kill processes that are currently communicating and such. We are working on it to fix the issues.
	 */
	public native void kill();
	public static void kill(Process p) {
		p.kill();
	}
	
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
	
	/** Yield the current process. All other processes that are ready at the same timestamp will be executed first */
	public static native void yield();
	
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
	 * @param pid			The process identifier of the process to get.
	 *
	 * @return				The process with the specified PID.
	 */
	public static native Process fromPID(int pid);
	/**
	 * This method returns the PID of the process.
	 *
	 * @return				The PID of the process.
	 *
	 */
	public int getPID()  {
		if (pid == -1) // Don't traverse the JNI barrier if you already have the answer
			pid = nativeGetPID();
		return pid;
	}
	// This should not be used: the PID is supposed to be initialized from the C directly when the actor is created,
	// but this sometimes fail, so let's play nasty but safe here.
	private native int nativeGetPID();
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
	 * Makes the current process sleep until millis milliseconds have elapsed.
	 * You should note that unlike "waitFor" which takes seconds (as usual in SimGrid), this method takes milliseconds (as usual for sleep() in Java).
	 * 
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
	 * @param nanos additional nanoseconds to sleep.
	 */
	public static native void sleep(long millis, int nanos) throws HostFailureException;
	/**
	 * Makes the current process sleep until time seconds have elapsed.
	 * @param seconds		The time the current process must sleep.
	 */
	public native void waitFor(double seconds) throws HostFailureException;
	/**
	 * This method actually creates and run the process.
	 * It is a noop if the process is already launched.
	 */
	public final void start() {
	   if (bind == 0)
	     create(host);
	}

	/** This method runs the process. It calls the method function that you must overwrite. */
	@Override
	public void run() {

		try {
			String[] argsArray = new String[this.args.size()];
		        this.args.toArray(argsArray);

			this.main(argsArray);
		}
		catch(MsgException e) {
			e.printStackTrace();
			Msg.info("Unexpected behavior. Stopping now");
			System.exit(1);
		}
		/* Let the ProcessKilledError (that we'd get if the process is forcefully killed) flow back to the caller */
	}

	/**
	 * The main function of the process (to implement by the user).
	 *
	 * @param args
	 * @throws MsgException
	 */
	public abstract void main(String[]args) throws MsgException;

	/** Stops the execution of the current actor */
	public void exit() {
		this.kill();
	}
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
	public static native int getCount();

        public static void debugAllThreads() {
	    // Search remaining threads that are not main nor daemon
            for (Thread t : Thread.getAllStackTraces().keySet())
                if (! t.isDaemon() && !t.getName().equals("main"))
                    System.err.println("Thread "+t.getName()+" is still running! Please report that bug");
        }
}
