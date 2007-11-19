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
 * Since most scheduling algorithms rely on a concept of 
 * task that can be either computed locally or transferred 
 * on another processor, it seems to be the right level of 
 * abstraction for our purposes. A task may then be defined 
 * by a computing amount, a message size and some private
 * data. To transfer a task you use an instance of the class
 * Channel identified by an number.
 *
 * @author  Abdelmalek Cherier
 * @author  Martin Quinson
 * @since SimGrid 3.3
 */
public class Task {

        /**
	 * This attribute represents a bind between a java task object and
	 * a native task. Even if this attribute is public you must never
	 * access to it. It is set automaticatly during the build of the object.
	 */
  public long bind = 0;


  /* Default constructor (disabled) */
  protected Task() {
  }
  /* *              * *
   * * Constructors * *
         * *              * *//**
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
	 * @exception			JniException if the binding mecanism fails.
	 */ public Task(String name, double computeDuration,
                 double messageSize) throws JniException {
    MsgNative.taskCreate(this, name, computeDuration, messageSize);
  }
    /**
     * Construct an new parallel task with the specified processing amount and amount for each host
     * implied.
     *
     * @param name		The name of the parallel task.
     * @param hosts		The list of hosts implied by the parallel task.
     * @param computeDurations	The amount of operations to be performed by each host of @hosts.
     * @param messageSizes	A matrix describing the amount of data to exchange between hosts.
     * 
     * @exception		JniException if the binding mecanism fails.
     */ public Task(String name, Host[]hosts, double[]computeDurations,
                    double[]messageSizes) throws JniException {
    MsgNative.parallelTaskCreate(this, name, hosts, computeDurations,
                                 messageSizes);
  }
  /* *                   * *
   * * Getters / Setters * *
         * *                   * *//**
	 * This method gets the name of a task.
	 *
	 * @return				The name of the task.
	 *
	 * @exception			JniException if the binding mecanism fails.
	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 */ public String getName() throws JniException {
    return MsgNative.taskGetName(this);
  }
        /**
 	 * This method gets the sender of the task.
 	 *
 	 * @return				The sender of the task.
 	 *
	 * @exception			JniException if the binding mecanism fails.
 	 *
 	 */ Process getSender() throws JniException {
    return MsgNative.taskGetSender(this);
  }
         /**
 	  * This method gets the source of the task.
 	  *
 	  * @return				The source of the task. 
 	  *
	  * @exception			JniException if the binding mecanism fails.
 	  */ public Host getSource() throws JniException, NativeException {
    return MsgNative.taskGetSource(this);
  }
        /**
	 * This method gets the computing amount of the task.
	 *
	 * @return				The computing amount of the task.
	 *
	 * @exception			JniException if the binding mecanism fails.
	 */ public double getComputeDuration() throws JniException {
    return MsgNative.taskGetComputeDuration(this);
  }
        /**
 	 * This method gets the remaining computation.
 	 *
 	 * @return				The remaining duration. 
 	 *
	 * @exception			JniException if the binding mecanism fails.
 	 */ public double getRemainingDuration() throws JniException {
    return MsgNative.taskGetRemainingDuration(this);
  }
        /**
 	 * This method sets the priority of the computation of the task.
 	 * The priority doesn't affect the transfert rate. For example a
 	 * priority of 2 will make the task receive two times more cpu than
 	 * the other ones.
 	 *
 	 * @param				The new priority of the task.
 	 *
 	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 */ public void setPrirority(double priority) throws JniException {
    MsgNative.taskSetPriority(this, priority);
  }
  /* *                       * *
   * * Communication-related * *
    * *                       * *//**
    * Retrieves next task on given channel of local host
    *
    * @exception  JniException if the binding mecanism fails.
    * @exception  NativeException if the retrival fails.
    */
    public static Task get(int channel) throws JniException, NativeException {
    return MsgNative.taskGet(channel, -1.0, null);
  }
   /**
    * Retrieves next task on given channel of local host (wait at most #timeout seconds)
    *
    * @exception  JniException if the binding mecanism fails.
    * @exception  NativeException if the retrival fails.
    */
    public static Task get(int channel, double timeout) throws JniException,
    NativeException {
    return MsgNative.taskGet(channel, timeout, null);
  }
   /**
    * Retrieves next task from given host on given channel of local host
    *
    * @exception  JniException if the binding mecanism fails.
    * @exception  NativeException if the retrival fails.
    */
    public static Task get(int channel, Host host) throws JniException,
    NativeException {
    return MsgNative.taskGet(channel, -1, host);
  }
   /**
    * Retrieves next task from given host on given channel of local host (wait at most #timeout seconds)
    *
    * @exception  JniException if the binding mecanism fails.
    * @exception  NativeException if the retrival fails.
    */ public static Task get(int channel, double timeout,
                              Host host) throws JniException, NativeException {
    return MsgNative.taskGet(channel, timeout, host);
  }
   /**
    * Probes whether there is a waiting task on the given channel of local host
    *
    * @exception  JniException if the binding mecanism fails.
    * @exception  NativeException if the retrival fails.
    */ public static boolean probe(int channel) throws JniException {
    return MsgNative.taskProbe(channel);
  }
   /**
    * Counts tasks waiting on the given #channel of local host and sent by given #host
    *
    * @exception  JniException if the binding mecanism fails.
    */ public static int probe(int channel, Host host) throws JniException {
    return MsgNative.taskProbeHost(channel, host);
  }
  /* *                     * *
   * * Computation-related * *
         * *                     * *//**
	 * This method execute a task on the location on which the
   	 * process is running.
   	 *
	 * @exception JniException if the binding mecanism fails.
	 * @exception NativeException if the cancelation failed.
 	 */ public void execute() throws JniException, NativeException {
    MsgNative.taskExecute(this);
  }
        /**
	 * This method cancels a task.
	 *
	 * @exception JniException if the binding mecanism fails.
	 * @exception NativeException if the cancelation failed.
	 */ public void cancel() throws JniException, NativeException {
    MsgNative.taskCancel(this);
  }
        /**
	 * This method deletes a task.
	 *
	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 *						MsgException if the destruction failed.
	 */ protected void finalize() throws JniException, NativeException {
    if (this.bind != 0)
      MsgNative.taskDestroy(this);
}}
