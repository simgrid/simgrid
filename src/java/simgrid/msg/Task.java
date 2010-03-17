/*
 * simgrid.msg.Task.java	1.00 07/05/01
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */

package simgrid.msg;

/**
 * A task is either something to compute somewhere, or something to exchange between two hosts (or both).
 * It is defined by a computing amount and a message size.
 *
 */
public class Task {
	/**
	 * This attribute represents a bind between a java task object and
	 * a native task. Even if this attribute is public you must never
	 * access to it. It is set automatically during the build of the object.
	 */
	public long bind = 0;


	/** Default constructor (all fields to 0 or null) */
	public Task() {
		MsgNative.taskCreate(this, null, 0, 0);
	}
	/* *              * *
	 * * Constructors * *
	 * *              * */
	/**
	 * Construct an new task with the specified processing amount and amount
	 * of data needed.
	 *
	 * @param name	Task's name
	 *
	 * @param computeDuration 	A value of the processing amount (in flop) needed to process the task. 
	 *				If 0, then it cannot be executed with the execute() method.
	 *				This value has to be >= 0.
	 *
	 * @param messageSize		A value of amount of data (in bytes) needed to transfert this task.
	 *				If 0, then it cannot be transfered with the get() and put() methods.
	 *				This value has to be >= 0.
	 */ 
	public Task(String name, double computeDuration, double messageSize) {
		MsgNative.taskCreate(this, name, computeDuration, messageSize);
	}
	/**
	 * Construct an new parallel task with the specified processing amount and amount for each host
	 * implied.
	 *
	 * @param name		The name of the parallel task.
	 * @param hosts		The list of hosts implied by the parallel task.
	 * @param computeDurations	The amount of operations to be performed by each host of \a hosts.
	 * @param messageSizes	A matrix describing the amount of data to exchange between hosts.
	 */ 
	public Task(String name, Host[]hosts, double[]computeDurations, double[]messageSizes) {
		MsgNative.parallelTaskCreate(this, name, hosts, computeDurations, messageSizes);
	}
	/* *                   * *
	 * * Getters / Setters * *
	 * *                   * */
	/** Gets the name of a task */ 
	public String getName() {
		return MsgNative.taskGetName(this);
	}
	/** Gets the sender of the task */ 
	Process getSender() {
		return MsgNative.taskGetSender(this);
	}
	/** Gets the source of the task */ 
	public Host getSource() throws NativeException {
		return MsgNative.taskGetSource(this);
	}
	/** Gets the computing amount of the task */ 
	public double getComputeDuration() {
		return MsgNative.taskGetComputeDuration(this);
	}
	/** Gets the remaining computation of the task */ 
	public double getRemainingDuration() {
		return MsgNative.taskGetRemainingDuration(this);
	}
	/**
	 * This method sets the priority of the computation of the task.
	 * The priority doesn't affect the transfert rate. For example a
	 * priority of 2 will make the task receive two times more cpu than
	 * the other ones.
	 *
	 * @param priority	The new priority of the task.
	 */ 
	public void setPriority(double priority) {
		MsgNative.taskSetPriority(this, priority);
	}
	/* *                       * *
	 * * Communication-related * *
	 * *                       * */


	/* *                     * *
	 * * Computation-related * *
	 * *                     * */
	/**
	 * Executes a task on the location on which the process is running.
	 *
	 * @exception NativeException if the execution failed.
	 */ 
	public void execute() throws NativeException {
		MsgNative.taskExecute(this);
	}
	/**
	 * Cancels a task.
	 *
	 * @exception NativeException if the cancellation failed.
	 */ 
	public void cancel() throws NativeException {
		MsgNative.taskCancel(this);
	}
	/** Deletes a task.
	 *
	 * @exception			NativeException if the destruction failed.
	 */ 
	protected void finalize() throws NativeException {
		if (this.bind != 0)
			MsgNative.taskDestroy(this);
	}

	/**
	 * Sends the task on the mailbox identified by the specified name 
	 *
	 * @exception  NativeException if the retrieval fails.
	 * @throws TimeoutException 
	 * @throws HostFailureException 
	 * @throws TransferFailureException 
	 */
	public void send(String mailbox) throws NativeException, TransferFailureException, HostFailureException, TimeoutException {
		MsgNative.taskSend(mailbox, this, -1);
	} 

	/**
	 * Sends the task on the mailbox identified by the specified name (wait at most \a timeout seconds)
	 *
	 * @exception  NativeException if the retrieval fails.
	 * @throws TimeoutException 
	 * @throws HostFailureException 
	 * @throws TransferFailureException 
	 */
	public void send(String mailbox, double timeout) throws NativeException, TransferFailureException, HostFailureException, TimeoutException {
		MsgNative.taskSend(mailbox, this, timeout);
	} 

	/**
	 * Sends the task on the mailbox identified by the specified alias  (capping the sending rate to \a maxrate) 
	 *
	 * @exception  NativeException if the retrieval fails.
	 */
	public void sendBounded(String alias, double maxrate) throws NativeException {
		MsgNative.taskSendBounded(alias, this, maxrate);
	} 

	/**
	 * Retrieves next task from the mailbox identified by the specified name
	 *
	 * @exception  NativeException if the retrieval fails.
	 */

	public static Task receive(String mailbox) throws NativeException {
		return MsgNative.taskReceive(mailbox, -1.0, null);
	}

	/**
	 * Retrieves next task on the mailbox identified by the specified name (wait at most \a timeout seconds)
	 *
	 * @exception  NativeException if the retrieval fails.
	 */
	public static Task receive(String mailbox, double timeout) throws  NativeException {
		return MsgNative.taskReceive(mailbox, timeout, null);
	}

	/**
	 * Retrieves next task sent by a given host on the mailbox identified by the specified alias 
	 *
	 * @exception  NativeException if the retrieval fails.
	 */

	public static Task receive(String mailbox, Host host) throws NativeException {
		return MsgNative.taskReceive(mailbox, -1.0, host);
	}

	/**
	 * Retrieves next task sent by a given host on the mailbox identified by the specified alias (wait at most \a timeout seconds)
	 *
	 * @exception  NativeException if the retrieval fails.
	 */
	public static Task receive(String mailbox, double timeout, Host host) throws NativeException {
		return MsgNative.taskReceive(mailbox, timeout, host);
	}

	/**
	 * Tests whether there is a pending communication on the mailbox identified by the specified alias, and who sent it
	 *
	 * @exception  NativeException if the retrieval fails.
	 */ 
	public static int listenFrom(String mailbox) throws NativeException  {
		return MsgNative.taskListenFrom(mailbox);
	}
	/**
	 * Listen whether there is a waiting task on the mailbox identified by the specified alias
	 *
	 * @exception  NativeException if the retrieval fails.
	 */ 
	public static boolean listen(String mailbox) throws NativeException  {
		return MsgNative.taskListen(mailbox);
	}

	/**
	 * Counts the number of tasks waiting to be received on the \a mailbox identified by the specified alia and sended by the specified \a host.
	 *
	 * @exception  NativeException if the retrieval fails.
	 */ 
	public static int listenFromHost(String alias, Host host) throws NativeException  {
		return MsgNative.taskListenFromHost(alias, host);
	}
}
