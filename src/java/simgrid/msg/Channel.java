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

/**
 * For convenience, the simulator provides the notion of
 * channel that is close to the tag notion in MPI. A channel
 * is not a socket. It doesn't need to be opened neither closed.
 * It rather corresponds to the ports opened on the different
 * machines.
 * 
 * @author  Abdelmalek Cherier
 * @author  Martin Quinson
 * @version $Id$
 * @see Task
 * @since SimGrid 3.3
 * @since JDK1.5011
 */
public final class Channel
{
	/**
	 * The channel identifier (as a port number in the TCP protocol.
	 */
	private int id;
	
	private Channel() {} /* disable the default constructor */
	
	/**
	 * Construct the channel with the specified identifier.
	 *
	 * @param id			The channel identifier.
	 */
	public Channel(int id){
		this.id = id;
	}
	
	/**
	 * This static method sets the number of channels for all the hosts
	 * of the simulation.
	 *
	 * param channelNumber	The channel number to set.
	 */		
	public static void setNumber(int channelNumber) {
		Msg.channelSetNumber(channelNumber);
	}
	
	/**
	 * This static method gets the number of channel of the simulation.
	 *
	 * @return				The channel numbers used in the simulation.
	 */
	public static int getNumber() {
		return Msg.channelGetNumber();
	}
	
	/** Returns the identifier of the channel */
	public int getId() {
		return this.id;
	}

	/**
	 * Listen on the channel and wait for receiving a task.
	 *
	 * @return				The task readed from the channel.
	 *
	 */
	public Task get() throws JniException,NativeException {
		return Msg.channelGet(this);
	}
	
	/**
	 * Listen on the channel and wait for receiving a task with a timeout.
	 *
	 * @param timeout		The timeout of the listening.
	 *
	 * @return				The task readed from the channel.
	 *
	 * @exception			NativeException if the listening operation failed.
	 */
	public Task getWithTimeout(double timeout) throws JniException, NativeException{
		return Msg.channelGetWithTimeout(this,timeout);
	}
	
	/**
	 * Listen on the channel and wait for receiving a task from a host.
	 *
	 * @param host			The host.
	 *
	 * @return				The task readed from the channel.
	 *
	 * @exception			InvalidHostException if the specified host is not valid.
	 *						MsgException if the listening operation failed.
	 *
	 * @see					Host
	 */
	public Task getFromHost(Host host) throws NativeException, JniException{
		return Msg.channelGetFromHost(this,host);
	}
	
	/**
	 * This method tests whether there is a pending communication on the channel.
	 *
	 * @return				This method returns true if there is a pending communication
	 *						on the channel. Otherwise the method returns false.
	 */
	public boolean hasPendingCommunication() throws NativeException, JniException{
		return Msg.channelHasPendingCommunication(this);
	}
	
	/**
	 * This method tests whether there is a pending communication on a 
	 * channel, and who sent it.
	 *
	 * @return				The method returns -1 if there is no pending 
	 *						communication and the PID of the process who sent it otherwise
	 */
	public int getCommunicatingProcess() throws JniException{
		return Msg.channelGetCommunicatingProcess(this) ;
	}
	
	/**
	 * Wait for at most timeout seconds for a task reception
	 * on channel. The PID is updated with the PID of the first process.
	 *
	 * @param timeout		The maximum time to wait for a task before
	 * 						giving up. 
	 * @return				The PID of the first process to send a task.
	 *
	 * @exception			MsgException if the reception failed.
	 */			
	public int wait(double timeout) throws NativeException, JniException{
		return Msg.channelWait(this,timeout);
	}
	
	/**
	 * This method returns the number of tasks waiting to be received on a
	 * channel and sent by a host.
	 *
	 * @param host			The host that is to be watched.
	 *
	 * @return				The number of tasks waiting to be received on a channel
	 * 						and sent by a host.
	 *
	 * @exception			InvalidHostException if the specified host is not valid.
	 */
	public int getHostWaitingTasks(Host host)  throws JniException{
		return Msg.channelGetHostWaitingTasks(this,host);
	}
	
	/**
	 * This method puts a task on a channel of an host and waits for the end of the 
	 * transmission.
	 *
	 * @param task			The task to put.
	 * @param host			The destinated host.
	 *
	 * @exception			InvalidTaskException if the task is not valid.
	 *						InvalidHostException if the host is not valid.
	 *						MsgException if the operation failed.
	 */				
	public void put(Task task,Host host) throws NativeException, JniException {
		Msg.channelPut(this,task,host);
	}
	
	/**
	 * This method puts a task on a channel of an  host (with a timeout on the waiting 
	 * of the destination host) and waits for the end of the transmission.
	 *
	 * @param task			The task to put.
	 * @param host			The destination where to put the task.
	 * @param timeout		The timeout of the transmission.
	 *
	 * @exception			InvalidTaskException if the task is not valid.
	 *						InvalidHostException if the host is not valid.
	 *						MsgException if the operation failed.
	 */
	public void putWithTimeout(Task task,Host host,double timeout) throws NativeException, JniException {
		Msg.channelPutWithTimeout(this,task,host,timeout);
	}
	
	/**
	 * This method does exactly the same as put() but with a bounded transmition
	 * rate.
	 *
	 * @param task			The task to put.
	 * @param host			The destination where to put the task.
	 * @param maxRate		The bounded transmition rate.
	 *
	 * @exception			InvalidTaskException if the task is not valid.
	 *						InvalidHostException if the host is not valid.
	 *						MsgException if the operation failed.
	 */
	public void putBounded(Task task,Host host,double maxRate) throws NativeException, JniException {
		Msg.channelPutBounded(this,task,host,maxRate);
	}
}
