/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */

package org.simgrid.msg;
 
import java.util.Arrays;
import java.util.Hashtable;
import java.util.Vector;
import java.util.concurrent.Semaphore;

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

public abstract class Process extends Thread {
	/**
	 * This attribute represents a bind between a java process object and
	 * a native process. Even if this attribute is public you must never
	 * access to it. It is set automatically during the build of the object.
	 */
	public long bind;

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
	public long id;

    public Hashtable<String,String> properties;

	/**
	 * The name of the process.							
	 */
	protected String name;
	/**
	  * The PID of the process
	  */
	protected int pid = -1;
	/**
	 * The PPID of the process 
	 */
	protected int ppid = -1;
	/**
	 * The host of the process
	 */
	protected Host host = null;

	/** The arguments of the method function of the process. */     
	public Vector<String> args;

	/* process synchronization tools */
	
	/* give the full path to semaphore to ensure that our own implementation don't get selected */
    protected java.util.concurrent.Semaphore schedBegin, schedEnd;
    private boolean nativeStop = false;

	/**
	 * Default constructor (used in ApplicationHandler to initialize it)
	 */
	protected Process() {
		super();
		this.id = nextProcessId++;
		this.name = null;
		this.bind = 0;
		this.args = new Vector<String>();
		this.properties = null;
		schedBegin = new java.util.concurrent.Semaphore(0);
		schedEnd = new java.util.concurrent.Semaphore(0);
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
	 *
	 */
	public Process(Host host, String name, String[]args) {
		/* This is the constructor called by all others */
		this();
		
		if (name == null)
			throw new NullPointerException("Process name cannot be NULL");
		this.name = name;

		this.args = new Vector<String>();
		if (null != args)
			this.args.addAll(Arrays.asList(args));

		try {
			create(host.getName());
		} catch (HostNotFoundException e) {
			throw new RuntimeException("The impossible happened (yet again): the host that I have were not found",e);
		}
		
	}
	/**
	 * The natively implemented method to create an MSG process.
	 * @param host    A valid (binded) host where create the process.
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
	/**
	 * This method sets a flag to indicate that this thread must be killed. End user must use static method kill
	 *
	 * @return				
	 *			
	 */ 
	public void nativeStop() {
		nativeStop = true;
	}
	/**
	 * getter for the flag that indicates that this thread must be killed
	 *
	 * @return				
	 *			
	 */ 
	public boolean getNativeStop() {
		return nativeStop;
	}

	/**
	 * This method kill a process.
	 * @param process  the process to be killed.
	 *
	 */
	public void kill() {
		 nativeStop();
		 Msg.info("Process " + msgName() + " will be killed.");		  			 		 
	}

	/**
	 * Suspends the process by suspending the task on which it was
	 * waiting for the completion.
	 *
	 */
	public native void pause();
	/**
	 * Resumes a suspended process by resuming the task on which it was
	 * waiting for the completion.
	 *
	 *
	 */ 
	public native void restart();
	/**
	 * Tests if a process is suspended.
	 *
	 * @return				The method returns true if the process is suspended.
	 *						Otherwise the method returns false.
	 */ 
	public native boolean isSuspended();
	/**
	 * Returns the name of the process
	 */
	public String msgName() {
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
	 * This static method returns the currently running process.
	 *
	 * @return				The current process.
	 *
	 */ 
	public static native Process currentProcess();
	/**
	 * Kills a MSG process
	 * @param process Valid java process to kill
	 */
	final static native void kill(Process process);	
	/**
	 * Migrates a process to another host.
	 *
	 * @param process		The process to migrate.
	 * @param host			The host where to migrate the process.
	 *
	 */
	public static void migrate(Process process, Host host)  {
		MsgNative.processMigrate(process, host);
		process.host = null;
	}
	/**
	 * Makes the current process sleep until time seconds have elapsed.
	 *
	 * @param seconds		The time the current process must sleep.
	 *
	 * @exception			HostFailureException on error in the native SimGrid code
	 */ 
	public native void waitFor(double seconds) throws HostFailureException;    /**
     *
     */
    public void showArgs() {
		Msg.info("[" + this.name + "/" + this.getHost().getName() + "] argc=" +
				this.args.size());
		for (int i = 0; i < this.args.size(); i++)
			Msg.info("[" + this.msgName() + "/" + this.getHost().getName() +
					"] args[" + i + "]=" + (String) (this.args.get(i)));
	}
    /**
     * Exit the process
     */
    public native void exit();

	/**
	 * This method runs the process. Il calls the method function that you must overwrite.
	 */
	public void run() {

		String[] args = null;      /* do not fill it before the signal or this.args will be empty */

		//waitSignal(); /* wait for other people to fill the process in */


		try {
			schedBegin.acquire();
		} catch(InterruptedException e) {
		}

		try {
			args = new String[this.args.size()];
			if (this.args.size() > 0) {
				this.args.toArray(args);
			}

			this.main(args);
			exit();
			schedEnd.release();
		} catch(MsgException e) {
			e.printStackTrace();
			Msg.info("Unexpected behavior. Stopping now");
			System.exit(1);
		}
		 catch(ProcessKilled pk) {
			if (nativeStop) {
				try {
					exit();
				} catch (ProcessKilled pk2) {
					/* Ignore that other exception that *will* occur all the time. 
					 * This is because the C mechanic gives the control to the now-killed process 
					 * so that it does some garbage collecting on its own. When it does so here, 
					 * the Java thread checks when starting if it's supposed to be killed (to inform 
					 * the C world). To avoid the infinite loop or anything similar, we ignore that 
					 * exception now. This should be ok since we ignore only a very specific exception 
					 * class and not a generic (such as any RuntimeException).
					 */
					System.err.println(currentThread().getName()+": I ignore that other exception");					
				}
				Msg.info(" Process " + ((Process) Thread.currentThread()).msgName() + " has been killed.");						
				schedEnd.release();			
			}
			else {
				pk.printStackTrace();
				Msg.info("Unexpected behavior. Stopping now");
				System.exit(1);
			}
		}	
	}

	/**
	 * The main function of the process (to implement).
     *
     * @param args
     * @throws MsgException
     */
	public abstract void main(String[]args) throws MsgException;


    /** @brief Gives the control from the given user thread back to the maestro 
     * 
     * schedule() and unschedule() are the basis of interactions between the user threads 
     * (executing the user code), and the maestro thread (executing the platform models to decide 
     * which user thread should get executed when. Once it decided which user thread should be run 
     * (because the blocking action it were blocked onto are terminated in the simulated world), the 
     * maestro passes the control to this uthread by calling uthread.schedule() in the maestro thread 
     * (check its code for the simple semaphore-based synchronization schema). 
     * 
     * The uthread executes (while the maestro is blocked), until it starts another blocking 
     * action, such as a communication or so. In that case, uthread.unschedule() gets called from 
     * the user thread.    
     *
     * As other complications, these methods are called directly by the C through a JNI upcall in 
     * response to the JNI downcalls done by the Java code. For example, you have this (simplified) 
     * execution path: 
     *   - a process calls the Task.send() method in java
     *   - this calls Java_org_simgrid_msg_MsgNative_taskSend() in C through JNI
     *   - this ends up calling jprocess_unschedule(), still in C
     *   - this calls the java method "org/simgrid/msg/Process/unschedule()V" through JNI
     *   - that is to say, the unschedule() method that you are reading the documentation of.
     *   
     * To understand all this, you must keep in mind that there is no difference between the C thread 
     * describing a process, and the Java thread doing the same. Most of the time, they are system 
     * threads from the kernel anyway. In the other case (such as when using green java threads when 
     * the OS does not provide any thread feature), I'm unsure of what happens: it's a very long time 
     * that I didn't see any such OS. 
     * 
     * The synchronization itself is implemented using simple semaphores in Java, as you can see by
     * checking the code of these functions (and run() above). That's super simple, and thus welcome
     * given the global complexity of the synchronization architecture: getting C and Java cooperate
     * with regard to thread handling in a portable manner is very uneasy. A simple and straightforward 
     * implementation of each synchronization point is precious. 
     *  
     * But this kinda limits the system scalability. It may reveal difficult to simulate dozens of 
     * thousands of processes this way, both for memory limitations and for hard limits pushed by the 
     * system on the amount of threads and semaphores (we have 2 semaphores per user process).
     * 
     * At time of writing, the best source of information on how to simulate large systems within the 
     * Java bindings of simgrid is here: http://tomp2p.net/dev/simgrid/
     * 
     */
    public void unschedule() {
    	/* this function is called from the user thread only */
		try {  	  
			
			/* unlock the maestro before going to sleep */
			schedEnd.release();
			/* Here, the user thread is locked, waiting for the semaphore, and maestro executes instead */
			schedBegin.acquire();
			/* now that the semaphore is acquired, it means that maestro gave us the control back */
			
			/* the user thread is starting again after giving the control to maestro. 
			 * Let's check if we were asked to die in between */
			if ( (Thread.currentThread() instanceof Process) &&((Process) Thread.currentThread()).getNativeStop()) {				
				throw new ProcessKilled();
			}
			
		} catch (InterruptedException e) {
			/* ignore this exception because this is how we get killed on process.kill or end of simulation.
			 * I don't like hiding exceptions this way, but fail to see any other solution 
			 */
		}
		
	}

    /** @brief Gives the control from the maestro back to the given user thread 
     * 
     * Must be called from the maestro thread -- see unschedule() for details.
     *
     */
    public void schedule() {
		try {
			/* unlock the user thread before going to sleep */
			schedBegin.release();
			/* Here, maestro is locked, waiting for the schedEnd semaphore to get signaled by used thread, that executes instead */
			schedEnd.acquire();
			/* Maestro now has the control back and the user thread went to sleep gently */
			
		} catch(InterruptedException e) {
			throw new RuntimeException("The impossible did happend once again: I got interrupted in schedEnd.acquire()",e);
		}
	}
    
	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		nativeInit();
	}
}
