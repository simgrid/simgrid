/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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

	/** Default constructor (all fields to 0 or null) */
	public Task() {
		create(null, 0, 0);
		this.messageSize = 0;
	}

	/* *              * *
	 * * Constructors * *
	 * *              * */
	/**
	 * Construct a new task with the specified processing amount and amount
	 * of data needed.
	 *
	 * @param name	Task's name
	 *
	 * @param flopsAmount 	A value of the processing amount (in flop) needed to process the task. 
	 *				If 0, then it cannot be executed with the execute() method.
	 *				This value has to be &ge; 0.
	 *
	 * @param bytesAmount		A value of amount of data (in bytes) needed to transfert this task.
	 *				If 0, then it cannot be transfered with the get() and put() methods.
	 *				This value has to be &ge; 0.
	 */ 
	public Task(String name, double flopsAmount, double bytesAmount) {
		create(name, flopsAmount, bytesAmount);
		this.messageSize = bytesAmount;
	}
	/**
	 * Construct a new parallel task with the specified processing amount and amount for each host
	 * implied.
	 *
	 * @param name		The name of the parallel task.
	 * @param hosts		The list of hosts implied by the parallel task.
	 * @param flopsAmount	The amount of operations to be performed by each host of hosts. 
	 *                      flopsAmount[i] is the total number of operations that have to be 
	 *                      performed on hosts[i].
	 * @param bytesAmount	A matrix describing the amount of data to exchange between hosts. The 
	 *                      length of this array must be hosts.length * hosts.length. It is actually 
	 *                      used as a matrix with the lines being the source and the columns being 
	 *                      the destination of the communications.
	 */ 
	public Task(String name, Host[]hosts, double[]flopsAmount, double[]bytesAmount) {
		parallelCreate(name, hosts, flopsAmount, bytesAmount);
	}

	/**
	 * The natively implemented method to create a MSG task.
	 *
	 * @param name            The name of the task.
	 * @param flopsAmount    A value of the processing amount (in flop) needed 
	 *                        to process the task. If 0, then it cannot be executed
	 *                        with the execute() method. This value has to be >= 0.
	 * @param bytesAmount        A value of amount of data (in bytes) needed to transfert 
	 *                        this task. If 0, then it cannot be transfered this task. 
	 *                        If 0, then it cannot be transfered with the get() and put() 
	 *                        methods. This value has to be >= 0.
	 * @exception             IllegalArgumentException if compute duration <0 or message size <0
	 */
	private final native void create(String name,
			double flopsAmount,
			double bytesAmount)
					throws IllegalArgumentException;		
	/**
	 * The natively implemented method to create a MSG parallel task.
	 *
	 * @param name                The name of the parallel task.
	 * @param hosts               The list of hosts implied by the parallel task.
	 * @param flopsAmount         The total number of operations that have to be performed
	 *                            on the hosts.
	 * @param bytesAmount        An array of doubles
	 *
	 */
	private final native void parallelCreate(String name,
			Host[]hosts,
			double[]flopsAmount,
			double[]bytesAmount)
					throws NullPointerException, IllegalArgumentException;
	/* *                   * *
	 * * Getters / Setters * *
	 * *                   * */
	/** Gets the name of the task */
	public String getName() {
		return name;
	}

	/** Gets the sender of the task (or null if not sent yet) */
	public native Process getSender();

	/** Gets the source of the task (or null if not sent yet). */
	public native Host getSource();   

	/** Gets the remaining amount of flops to execute in this task
	 * 
	 * If it's ongoing, you get the exact amount at the present time. If it's already done, it's 0.
	 */
	public native double getFlopsAmount();
	/**
	 * Sets the name of the task
	 * @param name the new task name
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

	/** Set the computation amount needed to process the task
	 * 
	 * Warning if the execution is already started and ongoing, this call does nothing.
	 * @param flopsAmount the amount of computation needed to process the task
	 */
	public native void setFlopsAmount(double flopsAmount);
	/**
	 * Set the amount of bytes to exchange the task
	 * 
	 * Warning if the communication is already started and ongoing, this call does nothing.
	 * @param bytesAmount the size of the task
	 */
	public native void setBytesAmount(double bytesAmount);
	/* *                     * *
	 * * Computation-related * *
	 * *                     * */
	/**
	 * Executes a task on the location on which the current process is running.
	 *
	 * @throws HostFailureException
	 * @throws TaskCancelledException
	 */
	public native void execute() throws HostFailureException,TaskCancelledException;

	/** Changes the maximum CPU utilization of a computation task. Unit is flops/s. */
	public native void setBound(double bound); 

	/** Cancels a task. */ 
	public native void cancel();

	/** Deletes a task once the garbage collector reclaims it */
	@Override
	protected void finalize() {
		try {
			// Exceptions in finalizers lead to bad situations:
			// http://stackoverflow.com/questions/7644556/troubleshooting-a-java-memory-leak-finalization
			nativeFinalize();
			bind=0; // to avoid segfaults if the impossible happens yet again making this task surviving its finalize()
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}
	protected native void nativeFinalize();
	/* *                       * *
	 * * Communication-related * *
	 * *                       * */

	/** Send the task asynchronously on the specified mailbox, 
	 *  with no way to retrieve whether the communication succeeded or not
	 * 
	 */
	public native void dsendBounded(String mailbox, double maxrate);


	/** Send the task asynchronously on the specified mailbox, 
	 *  with no way to retrieve whether the communication succeeded or not
	 * 
	 */
	public native void dsend(String mailbox);

	/**
	 * Sends the task on the specified mailbox 
	 *
	 * @param mailbox where to send the message
	 * @throws TimeoutException
	 * @throws HostFailureException 
	 * @throws TransferFailureException 
	 */
	public void send(String mailbox) throws TransferFailureException, HostFailureException, TimeoutException, NativeException {
		send(mailbox, -1);
	} 

	/**
	 * Sends the task on the specified mailbox (wait at most \a timeout seconds)
	 *
	 * @param mailbox where to send the message
	 * @param timeout
	 * @exception  NativeException if the retrieval fails.
	 * @throws TimeoutException 
	 * @throws HostFailureException 
	 * @throws TransferFailureException 
	 */
	public native void send(String mailbox, double timeout) throws TransferFailureException, HostFailureException, TimeoutException, NativeException;

	/** Sends the task on the specified mailbox (capping the sending rate to \a maxrate) 
	 *
	 * @param mailbox where to send the message
	 * @param maxrate 
	 * @throws TransferFailureException
	 * @throws HostFailureException
	 * @throws TimeoutException
	 */
	public void sendBounded(String mailbox, double maxrate) throws TransferFailureException, HostFailureException, TimeoutException {
		sendBounded(mailbox,-1,maxrate);
	}


	/** Sends the task on the specified mailbox (capping the sending rate to \a maxrate) with a timeout
	 *
	 * @param mailbox where to send the message
	 * @param timeout
	 * @param maxrate 
	 * @throws TransferFailureException
	 * @throws HostFailureException
	 * @throws TimeoutException
	 */
	public native void sendBounded(String mailbox, double timeout, double maxrate) throws TransferFailureException, HostFailureException, TimeoutException;


	/**
	 * Sends the task on the mailbox asynchronously
	 */
	public native Comm isend(String mailbox);

	/**
	 * Sends the task on the mailbox asynchronously (capping the sending rate to \a maxrate)
	 */
	public native Comm isendBounded(String mailbox, double maxrate);


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
	 * Starts listening for receiving a task from an asynchronous communication with a capped rate
	 * @param mailbox
	 */
	public static native Comm irecvBounded(String mailbox, double rate);
	/**
	 * Retrieves next task from the mailbox identified by the specified name with a capped rate
	 *
	 * @param mailbox
	 */

	public static Task receiveBounded(String mailbox, double rate) throws TransferFailureException, HostFailureException, TimeoutException {
		return receiveBounded(mailbox, -1.0, null, rate);
	}

	/**
	 * Retrieves next task on the mailbox identified by the specified name (wait at most \a timeout seconds) with a capped rate
	 *
	 * @param mailbox
	 * @param timeout
	 */
	public static Task receiveBounded(String mailbox, double timeout, double rate) throws  TransferFailureException, HostFailureException, TimeoutException {
		return receiveBounded(mailbox, timeout, null, rate);
	}

	/**
	 * Retrieves next task sent by a given host on the mailbox identified by the specified alias with a capped rate
	 *
	 * @param mailbox
	 * @param host
	 */

	public static Task receiveBounded(String mailbox, Host host, double rate) throws TransferFailureException, HostFailureException, TimeoutException {
		return receiveBounded(mailbox, -1.0, host, rate);
	}

	/**
	 * Retrieves next task sent by a given host on the mailbox identified by the specified alias (wait at most \a timeout seconds)
	 * with a capped rate
	 *
	 * @param mailbox
	 * @param timeout 
	 * @param host
	 */
	public native static Task receiveBounded(String mailbox, double timeout, Host host, double rate) throws TransferFailureException, HostFailureException, TimeoutException;



	/**
	 * Tests whether there is a pending communication on the mailbox identified by the specified alias, and who sent it
	 */
	public native static int listenFrom(String mailbox);
	/**
	 * Listen whether there is a task waiting (either for a send or a recv) on the mailbox identified by the specified alias
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
		org.simgrid.NativeLib.nativeInit();
		nativeInit();
	}

	public double getMessageSize() {
		return this.messageSize;
	}
}
