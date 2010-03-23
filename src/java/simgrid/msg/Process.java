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

package simgrid.msg;

import java.util.Arrays;
import java.util.Hashtable;
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
	public String msgName() {
		return this.name;
	}
	/** The arguments of the method function of the process. */     
	public Vector<String> args;

	/* process synchronization tools */
	protected Sem schedBegin, schedEnd;

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
		schedBegin = new Sem(0);
		schedEnd = new Sem(0);
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

		MsgNative.processCreate(this, host);
	}


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
	public static int killAll(int resetPID) {
		return MsgNative.processKillAll(resetPID);
	}

	/**
	 * This method adds an argument in the list of the arguments of the main function
	 * of the process. 
	 *
	 * @param arg			The argument to add.
	 */
	@Deprecated
	protected void addArg(String arg) {
		args.add(arg);
	}

	/**
	 * Suspends the process by suspending the task on which it was
	 * waiting for the completion.
	 *
	 */
	public void pause() {
		MsgNative.processSuspend(this);
	}
	/**
	 * Resumes a suspended process by resuming the task on which it was
	 * waiting for the completion.
	 *
	 *
	 */ 
	public void restart()  {
		MsgNative.processResume(this);
	}
	/**
	 * Tests if a process is suspended.
	 *
	 * @return				The method returns true if the process is suspended.
	 *						Otherwise the method returns false.
	 */ 
	public boolean isSuspended() {
		return MsgNative.processIsSuspended(this);
	}
	/**
	 * Returns the host of a process.
	 *
	 * @return				The host instance of the process.
	 *
	 * @exception			NativeException on error in the native SimGrid code
	 *
	 */ 
	public Host getHost() {
		return MsgNative.processGetHost(this);
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
	public static Process fromPID(int PID) throws NativeException {
		return MsgNative.processFromPID(PID);
	}
	/**
	 * This method returns the PID of the process.
	 *
	 * @return				The PID of the process.
	 *
	 */ 
	public int getPID()  {
		return MsgNative.processGetPID(this);
	}
	/**
	 * This method returns the PID of the parent of a process.
	 *
	 * @return				The PID of the parent of the process.
	 *
	 */ 
	public int getPPID()  {
		return MsgNative.processGetPPID(this);
	}
	/**
	 * This static method returns the currently running process.
	 *
	 * @return				The current process.
	 *
	 */ 
	public static Process currentProcess()  {
		return MsgNative.processSelf();
	}
	/**
	 * Migrates a process to another host.
	 *
	 * @param host			The host where to migrate the process.
	 *
	 */
	public void migrate(Host host)  {
		MsgNative.processChangeHost(this, host);
	}
	/**
	 * Makes the current process sleep until time seconds have elapsed.
	 *
	 * @param seconds		The time the current process must sleep.
	 *
	 * @exception			HostFailureException on error in the native SimGrid code
	 */ 
	public static void waitFor(double seconds) throws HostFailureException {
		MsgNative.processWaitFor(seconds);
	} 
	public void showArgs() {
		try {
			Msg.info("[" + this.name + "/" + this.getHost().getName() + "] argc=" +
					this.args.size());
			for (int i = 0; i < this.args.size(); i++)
				Msg.info("[" + this.msgName() + "/" + this.getHost().getName() +
						"] args[" + i + "]=" + (String) (this.args.get(i)));
		} catch(MsgException e) {
			Msg.info("Damn JNI stuff");
			e.printStackTrace();
			System.exit(1);
		}
	}
	/**
	 * This method runs the process. Il calls the method function that you must overwrite.
	 */
	public void run() {

		String[]args = null;      /* do not fill it before the signal or this.args will be empty */

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
			MsgNative.processExit(this);
			schedEnd.release();
		} catch(MsgException e) {
			e.printStackTrace();
			Msg.info("Unexpected behavior. Stopping now");
			System.exit(1);
		}
	}

	/**
	 * The main function of the process (to implement).
	 */
	public abstract void main(String[]args) throws MsgException;


	public void unschedule() {
		try {
			schedEnd.release();
			schedBegin.acquire();
		} catch(InterruptedException e) {
		}
	}

	public void schedule() {
		try {
			schedBegin.release();
			schedEnd.acquire();
		} catch(InterruptedException e) {
		}
	}

	/** Send the given task in the mailbox associated with the specified alias  (waiting at most given time) 
	 * @throws TimeoutException 
	 * @throws HostFailureException 
	 * @throws TransferFailureException */
	public void taskSend(String mailbox, Task task, double timeout) throws TransferFailureException, HostFailureException, TimeoutException {
		MsgNative.taskSend(mailbox, task, timeout);
	}

	/** Send the given task in the mailbox associated with the specified alias
	 * @throws TimeoutException 
	 * @throws HostFailureException 
	 * @throws TransferFailureException */
	public void taskSend(String mailbox, Task task) throws  TransferFailureException, HostFailureException, TimeoutException {
		MsgNative.taskSend(mailbox, task, -1);
	}

	/** Receive a task on mailbox associated with the specified mailbox */
	public Task taskReceive(String mailbox) throws TransferFailureException, HostFailureException, TimeoutException {
		return MsgNative.taskReceive(mailbox, -1.0, null);
	}

	/** Receive a task on mailbox associated with the specified alias (waiting at most given time) */
	public Task taskReceive(String mailbox, double timeout) throws  TransferFailureException, HostFailureException, TimeoutException {
		return MsgNative.taskReceive(mailbox, timeout, null);
	}

	/** Receive a task on mailbox associated with the specified alias from given sender */
	public Task taskReceive(String mailbox, double timeout, Host host) throws  TransferFailureException, HostFailureException, TimeoutException {
		return MsgNative.taskReceive(mailbox, timeout, host);
	}

	/** Receive a task on mailbox associated with the specified alias from given sender*/
	public Task taskReceive(String mailbox, Host host) throws  TransferFailureException, HostFailureException, TimeoutException {
		return MsgNative.taskReceive(mailbox, -1.0, host);
	}
}
