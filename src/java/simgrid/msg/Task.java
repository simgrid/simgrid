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


	/* Default constructor (disabled) */
	protected Task() {}
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
	 *
	 * @exception			JniException if the binding mechanism fails.
	 */ 
	public Task(String name, double computeDuration, double messageSize) throws JniException {
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
	 * 
	 * @exception		JniException if the binding mecanism fails.
	 */ 
	public Task(String name, Host[]hosts, double[]computeDurations, double[]messageSizes) throws JniException {
		MsgNative.parallelTaskCreate(this, name, hosts, computeDurations, messageSizes);
	}
	/* *                   * *
	 * * Getters / Setters * *
	 * *                   * */
	/** gets the name of a task. 
	 * @exception			JniException if the binding mechanism fails.
	 */ 
	public String getName() throws JniException {
		return MsgNative.taskGetName(this);
	}
	/** gets the sender of the task.
	 * @exception			JniException if the binding mechanism fails.
	 *
	 */ 
	Process getSender() throws JniException {
		return MsgNative.taskGetSender(this);
	}
	/** Gets the source of the task.
	 * @exception			JniException if the binding mechanism fails.
	 */ 
	public Host getSource() throws JniException, NativeException {
		return MsgNative.taskGetSource(this);
	}
	/** gets the computing amount of the task.
	 * @exception			JniException if the binding mechanism fails.
	 */ 
	public double getComputeDuration() throws JniException {
		return MsgNative.taskGetComputeDuration(this);
	}
	/** gets the remaining computation of the task.
	 * @exception			JniException if the binding mechanism fails.
	 */ 
	public double getRemainingDuration() throws JniException {
		return MsgNative.taskGetRemainingDuration(this);
	}
	/**
	 * This method sets the priority of the computation of the task.
	 * The priority doesn't affect the transfert rate. For example a
	 * priority of 2 will make the task receive two times more cpu than
	 * the other ones.
	 *
	 * @param priority	The new priority of the task.
	 *
	 * @exception	JniException is the specified task is not valid (ie, not binded to a native task)
	 */ 
	public void setPriority(double priority) throws JniException {
		MsgNative.taskSetPriority(this, priority);
	}
	/* *                       * *
	 * * Communication-related * *
	 * *                       * */


	/* *                     * *
	 * * Computation-related * *
	 * *                     * */
	/**
	 * This method execute a task on the location on which the
	 * process is running.
	 *
	 * @exception JniException if the binding mechanism fails.
	 * @exception NativeException if the execution failed.
	 */ 
	public void execute() throws JniException, NativeException {
		MsgNative.taskExecute(this);
	}
	/**
	 * This method cancels a task.
	 *
	 * @exception JniException if the binding mechanism fails.
	 * @exception NativeException if the cancellation failed.
	 */ 
	public void cancel() throws JniException, NativeException {
		MsgNative.taskCancel(this);
	}
	/** Deletes a task.
	 *
	 * @exception			JniException if the binding mechanism fails
	 *						NativeException if the destruction failed.
	 */ 
	protected void finalize() throws JniException, NativeException {
		if (this.bind != 0)
			MsgNative.taskDestroy(this);
	}

	/**
	 * Send the task on the mailbox identified by the specified name 
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */
	public void send(String mailbox) throws JniException,NativeException {
		MsgNative.taskSend(mailbox, this, -1);
	} 

	/**
	 * Send the task on the mailbox identified by the specified name (wait at most \a timeout seconds)
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */
	public void send(String mailbox, double timeout) throws JniException,NativeException {
		MsgNative.taskSend(mailbox, this, timeout);
	} 

	/**
	 * Send the task on the mailbox identified by the specified alias  (capping the sending rate to \a maxrate) 
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */
	public void sendBounded(String alias, double maxrate) throws JniException,NativeException {
		MsgNative.taskSendBounded(alias, this, maxrate);
	} 

	/**
	 * Retrieves next task from the mailbox identified by the specified name
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */

	public static Task receive(String mailbox) throws JniException, NativeException {
		return MsgNative.taskReceive(mailbox, -1.0, null);
	}

	/**
	 * Retrieves next task on the mailbox identified by the specified name (wait at most \a timeout seconds)
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */
	public static Task receive(String mailbox, double timeout) throws JniException, NativeException {
		return MsgNative.taskReceive(mailbox, timeout, null);
	}

	/**
	 * Retrieves next task sent by a given host on the mailbox identified by the specified alias 
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */

	public static Task receive(String mailbox, Host host) throws JniException, NativeException {
		return MsgNative.taskReceive(mailbox, -1.0, host);
	}

	/**
	 * Retrieves next task sent by a given host on the mailbox identified by the specified alias (wait at most \a timeout seconds)
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */
	public static Task receive(String mailbox, double timeout, Host host) throws JniException, NativeException {
		return MsgNative.taskReceive(mailbox, timeout, host);
	}

	/**
	 * Test whether there is a pending communication on the mailbox identified by the specified alias, and who sent it
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */ 
	public static int listenFrom(String mailbox) throws JniException, NativeException  {
		return MsgNative.taskListenFrom(mailbox);
	}
	/**
	 * Listen whether there is a waiting task on the mailbox identified by the specified alias
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */ 
	public static boolean listen(String mailbox) throws JniException, NativeException  {
		return MsgNative.taskListen(mailbox);
	}

	/**
	 * Counts the number of tasks waiting to be received on the \a mailbox identified by the specified alia and sended by the specified \a host.
	 *
	 * @exception  JniException if the binding mechanism fails.
	 * @exception  NativeException if the retrieval fails.
	 */ 
	public static int listenFromHost(String alias, Host host) throws JniException, NativeException  {
		return MsgNative.taskListenFromHost(alias, host);
	}
}
