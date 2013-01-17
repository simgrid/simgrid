/*
 * Copyright 2006-2012 The SimGrid Team.           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */

package org.simgrid.msg;

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
	private long bind = 0;
	/**
	 * Task name
	 */
	protected String name;

	private double messageSize;

	static private Long idCpt = 0L;

	private Long id;

	/** Default constructor (all fields to 0 or null) */
	public Task() {
		create(null, 0, 0);
		this.messageSize = 0;
		setId(idCpt);
		idCpt++;
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
		create(name, computeDuration, messageSize);
		this.messageSize = messageSize;
		setId(idCpt);
		idCpt++;
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
		parallelCreate(name, hosts, computeDurations, messageSizes);
	}
	
	/**
	 * The natively implemented method to create a MSG task.
	 *
	 * @param name            The name of th task.
	 * @param computeDuration    A value of the processing amount (in flop) needed 
	 *                        to process the task. If 0, then it cannot be executed
	 *                        with the execute() method. This value has to be >= 0.
	 * @param messageSize        A value of amount of data (in bytes) needed to transfert 
	 *                        this task. If 0, then it cannot be transfered this task. 
	 *                        If 0, then it cannot be transfered with the get() and put() 
	 *                        methods. This value has to be >= 0.
	 * @exception             IllegalArgumentException if compute duration <0 or message size <0
	 */
	private final native void create(String name,
			double computeDuration,
			double messageSize)
	throws IllegalArgumentException;		
	/**
	 * The natively implemented method to create a MSG parallel task.
	 *
	 * @param name                The name of the parallel task.
	 * @param hosts                The list of hosts implied by the parallel task.
	 * @param computeDurations    The total number of operations that have to be performed
	 *                            on the hosts.
	 * @param messageSizes        An array of doubles
	 *
	 */
	private final native void parallelCreate(String name,
			Host[]hosts,
			double[]computeDurations,
			double[]messageSizes)
	throws NullPointerException, IllegalArgumentException;
	/* *                   * *
	 * * Getters / Setters * *
	 * *                   * */
    /** 
     * Gets the name of a task
     */
	public String getName() {
		return name;
	}
	/**
	 * Gets the sender of the task 
	 * Returns null if the task hasn't been sent yet
	 */
	public native Process getSender();
	/** Gets the source of the task.
	 * Returns null if the task hasn't been sent yet.
     */
	public native Host getSource();   
	/** Gets the computing amount of the task
     * FIXME: Cache it !
     */
	public native double getComputeDuration();
	/** Gets the remaining computation of the task
     */
	public native double getRemainingDuration();
	/**
	 * Sets the name of the task
	 * @param name the new task name.c
	 */
	public native void setName(String name);
	/**
	 * This method sets the priority of the computation of the task.
	 * The priority doesn't affect the transfer rate. For example a
	 * priority of 2 will make the task receive two times more cpu than
	 * the other ones.
	 *
	 * @param priority	The new priority of the task.
	 */ 
	public native void setPriority(double priority);
	/**
	 * Set the computation amount needed to process the task
	 * @param computationAmount the amount of computation needed to process the task
	 */
	public native void setComputeDuration(double computationAmount);
	/* *                     * *
	 * * Computation-related * *
	 * *                     * */
	/**
	 * Executes a task on the location on which the process is running.
	 *
     *
     * @throws HostFailureException
     * @throws TaskCancelledException
     */
	public native void execute() throws HostFailureException,TaskCancelledException;
	/**
	 * Cancels a task.
	 *
	 */ 
	public native void cancel();
	/** Deletes a task.
	 *
	 * @exception			NativeException if the destruction failed.
	 */ 
	protected void finalize() throws NativeException {
		destroy();
	}
	/**
	 * The natively implemented method to destroy a MSG task.
	 */
	protected native void destroy();
	/* *                       * *
	 * * Communication-related * *
	 * *                       * */

	/** Send the task asynchronously on the mailbox identified by the specified name, 
	 *  with no way to retrieve whether the communication succeeded or not
	 * 
	 */
	public native void dsend(String mailbox);	
	/**
	 * Sends the task on the mailbox identified by the specified name 
	 *
     * @param mailbox
     * @throws TimeoutException
	 * @throws HostFailureException 
	 * @throws TransferFailureException 
	 */
	public void send(String mailbox) throws TransferFailureException, HostFailureException, TimeoutException {
		send(mailbox, -1);
	} 

	/**
	 * Sends the task on the mailbox identified by the specified name (wait at most \a timeout seconds)
	 *
     * @param mailbox
     * @param timeout
     * @exception  NativeException if the retrieval fails.
	 * @throws TimeoutException 
	 * @throws HostFailureException 
	 * @throws TransferFailureException 
	 */
	public native void send(String mailbox, double timeout) throws TransferFailureException, HostFailureException, TimeoutException;
	/**
	 * Sends the task on the mailbox identified by the specified alias  (capping the sending rate to \a maxrate) 
	 *
     * @param alias
     * @param maxrate 
     * @throws TransferFailureException
     * @throws HostFailureException
     * @throws TimeoutException
	 */
	public native void sendBounded(String alias, double maxrate) throws TransferFailureException, HostFailureException, TimeoutException;
	/**
	 * Sends the task on the mailbox asynchronously
	 */
	public native Comm isend(String mailbox);
	
	/**
	 * Starts listening for receiving a task from an asynchronous communication
	 * @param mailbox
	 */
	public static native Comm irecv(String mailbox);
	/**
	 * Retrieves next task from the mailbox identified by the specified name
	 *
     * @param mailbox
	 */

	public static Task receive(String mailbox) throws TransferFailureException, HostFailureException, TimeoutException {
		return receive(mailbox, -1.0, null);
	}

	/**
	 * Retrieves next task on the mailbox identified by the specified name (wait at most \a timeout seconds)
	 *
     * @param mailbox
     * @param timeout
	 */
	public static Task receive(String mailbox, double timeout) throws  TransferFailureException, HostFailureException, TimeoutException {
		return receive(mailbox, timeout, null);
	}

	/**
	 * Retrieves next task sent by a given host on the mailbox identified by the specified alias 
	 *
     * @param mailbox
     * @param host
	 */

	public static Task receive(String mailbox, Host host) throws TransferFailureException, HostFailureException, TimeoutException {
		return receive(mailbox, -1.0, host);
	}

	/**
	 * Retrieves next task sent by a given host on the mailbox identified by the specified alias (wait at most \a timeout seconds)
	 *
     * @param mailbox
     * @param timeout 
     * @param host
	 */
	public native static Task receive(String mailbox, double timeout, Host host) throws TransferFailureException, HostFailureException, TimeoutException;
	/**
	 * Tests whether there is a pending communication on the mailbox identified by the specified alias, and who sent it
     */
	public native static int listenFrom(String mailbox);
	/**
	 * Listen whether there is a waiting task on the mailbox identified by the specified alias
     */
	public native static boolean listen(String mailbox);

	/**
	 * Counts the number of tasks waiting to be received on the \a mailbox identified by the specified alia and sended by the specified \a host.
     */
	public native static int listenFromHost(String alias, Host host);
	
	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		Msg.nativeInit();
		nativeInit();
	}

	public double getMessageSize() {
		return this.messageSize;
	}

	public Long getId() {
		return id;
	}

	public void setId(Long id) {
		this.id = id;
	}
}
