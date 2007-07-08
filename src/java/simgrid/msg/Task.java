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
 * @version 1.00, 07/05/01
 * @see Process
 * @see Channel
 * @since SimGrid 3.3
 * @since JDK1.5011 
 */
public class Task
{
	/**
	 * This attribute represents a bind between a java task object and
	 * a native task. Even if this attribute is public you must never
	 * access to it. It is set automaticatly during the build of the object.
	 */
	public long bind = 0;
	
	/**
	 * Default constructor (disabled)
	 */
	protected Task() {}
	
	/**
	 * Construct an new task with the specified processing amount and amount
	 * of data needed.
	 *
	 * @param name				The name of th task.
	 *
	 * @param computeDuration	A value of the processing amount (in flop) needed 
	 *							to process the task. If 0, then it cannot be executed
	 *							with the execute() method. This value has to be >= 0.
	 *
	 * @param messageSize		A value of amount of data (in bytes) needed to transfert 
	 *							this task. If 0, then it cannot be transfered this task. 
	 *							If 0, then it cannot be transfered with the get() and put() 
	 *							methods. This value has to be >= 0.
	 *
	 * @exception				InvalidTaskNameException if the specified name is null.
	 *							InvalidComputeDuration if the specified compute duration is less than 0.
	 *							InvalidMessageSizeException if the specified message size is less than 0.
	 */
	public Task(String name, double computeDuration, double messageSize) throws JniException {
			
		create(name,computeDuration,messageSize);		
	}
	
	/**
	 * This method creates a new task with the specified features. The task
	 * creation means that the native task is created and binded with the
	 * java task instance. If the task is already created you must destroy it
	 * before to use this method, otherwise the method throws the exception
	 * TaskAlreadyCreatedException.
	 *
	 * @param name				The name of the task.
	 * @param computeDuration	A value of the processing amount (in flop) needed 
	 *							to process the task. If 0, then it cannot be executed
	 *							with the execute() method. This value has to be >= 0.
	 * @param messageSize		A value of amount of data (in bytes) needed to transfert 
	 *							this task. If 0, then it cannot be transfered this task. 
	 *							If 0, then it cannot be transfered with the get() and put() 
	 *							methods. This value has to be >= 0.
	 *
	 * @exception				InvalidTaskNameException if the specified name is null.
	 *							InvalidComputeDuration if the specified compute duration is less than 0.
	 *							InvalidMessageSizeException if the specified message size is less than 0.
	 */
    public void create(String name, double computeDuration, double messageSize) throws JniException {
		
	if(this.bind == 0)
	    Msg.taskCreate(this,name,computeDuration,messageSize);
    }
	
	/**
 	 * This method gets the sender of the task.
 	 *
 	 * @return				The sender of the task.
 	 *
 	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 *
 	 */
	Process getSender() throws JniException{
		return Msg.taskGetSender(this);
	}
	
	 /**
 	  * This method gets the source of the task.
 	  *
 	  * @return				The source of the task. 
 	  *
 	  * @exception			InvalidTaskException is the specified task is not valid. A task
 	  *						is invalid if it is not binded with a native task.
 	  */
	public Host getSource()throws JniException, NativeException{
		return Msg.taskGetSource(this);
	}
	
	/**
	 * This method gets the name of a task.
	 *
	 * @return				The name of the task.
	 *
	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 */
	public String getName()throws JniException{
		return Msg.taskGetName(this);
	}
	
	/**
	 * This method cancels a task.
	 *
	 * @exception			InvalidTaskException if the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 *						MsgException if the cancelation failed.
	 */
	public void cancel() throws JniException, NativeException{
		Msg.taskCancel(this);
	}
	
	/**
	 * This method gets the computing amount of the task.
	 *
	 * @return				The computing amount of the task.
	 *
	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
	 */
	public double getComputeDuration()throws JniException{
		return Msg.taskGetComputeDuration(this);
	}
	
	/**
 	 * This method gets the remaining computation.
 	 *
 	 * @return				The remaining duration. 
 	 *
	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 */	
	public double getRemainingDuration() throws JniException {
		return Msg.taskGetRemainingDuration(this);
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
 	 */
	public void setPrirority(double priority) throws JniException {
		 Msg.taskSetPriority(this,priority);
	}
	
	/**
	 * This method destroys a task.
	 *
	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 *						MsgException if the destruction failed.
	 */
	public void destroy()  throws JniException, NativeException {
		if(this.bind != 0) {
			Msg.taskDestroy(this);
			this.bind = 0;
		}
	}			
	
	/**
	 * This method deletes a task.
	 *
	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 *						MsgException if the destruction failed.
	 */
	protected void finalize() throws JniException, NativeException {
		if(this.bind != 0)
			Msg.taskDestroy(this);
	}
	
	/**
	 * This method execute a task on the location on which the
   	 * process is running.
   	 *
   	 * @exception			InvalidTaskException is the specified task is not valid. A task
 	 *						is invalid if it is not binded with a native task.
 	 *						MsgException if the destruction failed.
 	 */
	public void execute() throws JniException, NativeException {
		Msg.taskExecute(this);
	}					
}
