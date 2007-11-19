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

class ParallelTask {

    /**
     * This attribute represents a bind between a java task object and
     * a native task. Even if this attribute is public you must never
     * access to it. It is set automaticatly during the build of the object.
     */
  public long bind = 0;


  /* Default constructor (disabled) */
  protected ParallelTask() {
  };

    /**
     * Construct an new parallel task with the specified processing amount and amount for each host
     * implied.
     *
     * @param name				The name of the parallel task.
     * @param hosts				The list of hosts implied by the parallel task.
     * @param computeDurations	The total number of operations that have to be performed
     *							on the hosts.
     * @param messageSizes		An array of doubles
     * @exception				InvalidTaskNameException if the specified name is null.
     *							InvalidComputeDuration if the parameter computeDurations is null.
     *							InvalidMessageSize if the parameter messageSizes is null.
     */
  public ParallelTask(String name, Host[]hosts, double[]computeDurations,
                      double[]messageSizes)
  throws JniException {
    create(name, hosts, computeDurations, messageSizes);
  }
    /**
     * This method creates a parallel task (if not already created).
     *
     * @param name				The name of the parallel task.
     * @param hosts				The list of hosts implied by the parallel task.
     * @param computeDurations	The total number of operations that have to be performed
     *							on the hosts.
     * @param messageSizes		An array of doubles
     * @exception				InvalidTaskNameException if the specified name is null.
     *							InvalidComputeDuration if the parameter computeDurations is null.
     *							InvalidMessageSize if the parameter messageSizes is null.
     */
    public void create(String name, Host[]hosts, double[]computeDurations,
                         double[]messageSizes)
  throws JniException {

    if (bind != 0)
      Msg.parallelTaskCreate(this, name, hosts, computeDurations,
                             messageSizes);
  }
    /**
     * This method gets the sender of the parallel task.
     *
     * @return				The sender of the parallel task.
     *
     * @exception			InvalidTaskException is the specified parallel task is not valid. A parallel task
     *						is invalid if it is not binded with a native parallel task.
     *
     */ Process getSender() throws JniException {
    return Msg.parallelTaskGetSender(this);
  }
    /**
     * This method gets the source of the parallel task.
     *
     * @return				The source of the task. 
     *
     * @exception			InvalidTaskException is the specified parallel task is not valid. A task
     *						is invalid if it is not binded with a native parallel task.
     */ public Host getSource() throws JniException {
    return Msg.parallelTaskGetSource(this);
  }
    /**
     * This method gets the name of a parallel task.
     *
     * @return				The name of the parallel task.
     *
     * @exception			InvalidTaskException is the specified parallel task is not valid. A parallel task
     *						is invalid if it is not binded with a native parallel task.
     */ public String getName() throws JniException {
    return Msg.parallelTaskGetName(this);
  }
    /**
     * This method cancels a task.
     *
     * @exception			InvalidTaskException is the specified parallel task is not valid. A parallel task
     *						is invalid if it is not binded with a native parallel task.
     *						MsgException if the cancelation failed.
     */ public void cancel() throws JniException, NativeException {
    Msg.parallelTaskCancel(this);
  }
    /**
     * This method gets the computing amount of the parallel task.
     *
     * @return				The computing amount of the parallel task.
     *
     * @exception			InvalidTaskException is the specified task is not valid. A task
     *						is invalid if it is not binded with a native task.
     */ public double getComputeDuration() throws JniException {
    return Msg.parallelTaskGetComputeDuration(this);
  }
    /**
     * This method gets the remaining computation.
     *
     * @return				The remaining duration. 
     *
     * @exception			InvalidTaskException is the specified parallel task is not valid. A parallel task
     *						is invalid if it is not binded with a native parallel task.
     */ public double getRemainingDuration() throws JniException {
    return Msg.parallelTaskGetRemainingDuration(this);
  }
    /**
     * This method sets the priority of the computation of the parallel task.
     * The priority doesn't affect the transfert rate. For example a
     * priority of 2 will make the task receive two times more cpu than
     * the other ones.
     *
     * @param				The new priority of the parallel task.
     *
     * @exception			InvalidTaskException is the specified task is not valid. A task
     *						is invalid if it is not binded with a native task.
     */ public void setPrirority(double priority) throws JniException {
    Msg.parallelTaskSetPriority(this, priority);
  }
    /**
     * This method destroies a parallel task.
     *
     * @exception			InvalidTaskException is the specified task is not valid. A parallel task
     *						is invalid if it is not binded with a native task.
     *						MsgException if the destruction failed.
     */ public void destroy() throws JniException, NativeException {
    Msg.parallelTaskDestroy(this);
  }
    /**
     * This method deletes a parallel task.
     *
     * @exception			InvalidTaskException is the specified parallel task is not valid. A parallel task
     *						is invalid if it is not binded with a native parallel task.
     *						MsgException if the destruction failed.
     */ protected void finalize() throws JniException, NativeException {
    if (this.bind != 0)
      Msg.parallelTaskDestroy(this);
  }
    /**
     * This method execute a task on the location on which the
     * process is running.
     *
     * @exception			InvalidTaskException is the specified parallel task is not valid. A parallel task
     *						is invalid if it is not binded with a native parallel task.
     *						MsgException if the destruction failed.
     */ public void execute() throws JniException, NativeException {
    Msg.parallelTaskExecute(this);
}}
