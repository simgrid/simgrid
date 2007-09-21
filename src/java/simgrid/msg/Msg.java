/*
 * simgrid.msg.Msg.java    1.00 07/05/01
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */
 
package simgrid.msg;

import java.lang.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

/** 
 * MSG was the first distributed programming environment 
 * provided within SimGrid. While almost realistic, it 
 * remains quite simple.
 * This class contains all the declaration of the natives methods
 * of the MSG API.
 * All of these methods are statics. You can't directly use these methods
 * but it is recommanded to use the classes Process, Host, Task and ParallelTask
 * of the package to build your application.
 *
 * @author  Abdelmalek Cherier
 * @author  Martin Quinson
 * @version 1.00, 07/05/01
 * @see Host
 * @see Process
 * @see Task
 * @see ParallelTask
 * @since SimGrid 3.3
 * @since JDK1.5011 
 */
public final class Msg {
    /**
     * The natively implemented method to get the last error code of the simulation.
     *
     * @return     The last error code of the simulation.
     */
    final static native int getErrCode();
    
    /**
     * Errors returned by the method Msg.getErrCode().
     */
    
    /* Everything is right. Keep on goin the way !                                
     *
     */
    public static final int SUCCESS                = 0;
    
    /* Something must be not perfectly clean. But I 
     * may be paranoid freak... !    
     */
    public static final int WARNING                = 1;
    
    /* There has been a problem during you task treansfer.
     * Either the network is  down or the remote host has 
     * been shutdown.
     */
    public static final int TRANSFERT_FAILURE    = 2;
    
    /**
     * System shutdown. The host on which you are running
     * has just been rebooted. Free your datastructures and
     * return now !
     */
    public static final int HOST_FAILURE        = 3;
    
    /**
     * Cancelled task. This task has been cancelled by somebody !
     */
    public static final int TASK_CANCELLLED        = 4;
    
    /**
     * You've done something wrong. You'd better look at it...
     */
    public static final int FATAL_ERROR            = 5;
    
    /**
     * Staticaly load the SIMGRID4JAVA library
     *  which contains all native functions used in here
     */
    static {
        try {
            System.loadLibrary("libSimgrid4java");
        } catch(UnsatisfiedLinkError e){
            System.err.println("Cannot load simgrid4java library : ");
            e.printStackTrace();
	    System.err.println("Please check your LD_LIBRARY_PATH, "+
			       "or copy the library to the current directory");
	    System.exit(1);
        }
    }
    
    /******************************************************************
     * The natively implemented methods connected to the MSG Process  *
     ******************************************************************/
     
    /**
     * The natively implemented method to create an MSG process.
     *
     * @param process The java process object to bind with the MSG native process.
     * @param host    A valid (binded) host where create the process.
     *
     * @exception  simgrid.msg.JniException if JNI stuff goes wrong.
     *
     * @see  Process constructors.
     */
    final static native 
	void processCreate(Process process, Host host) throws JniException;
     
    /**
     * The natively implemented method to kill all the process of the simulation.
     *
     * @param resetPID        Should we reset the PID numbers. A negative number means no reset
     *                        and a positive number will be used to set the PID of the next newly
     *                        created process.
     *
     * @return                The function returns the PID of the next created process.
     */
    final static native int processKillAll(int resetPID);
       
    /**
     * The natively implemented method to suspend an MSG process.
     *
     * @param process        The valid (binded with a native process) java process to suspend.
     *
     * @exception            JniException if something goes wrong with JNI
     *                       NativeException if the SimGrid native code failed.
     *
     * @see                 Process.pause()
     */
    final static native void processSuspend(Process process) 
	throws JniException, NativeException;
     
    /**
     * The natively implemented method to kill a MSG process.
     *
     * @param process        The valid (binded with a native process) java process to kill.
     *
     * @see                 Process.kill()
     */
    final static native void processKill(Process process)
	throws JniException;
     
    /**
     * The natively implemented method to resume a suspended MSG process.
     *
     * @param process        The valid (binded with a native process) java process to resume.
     *
     * @exception            JniException if something goes wrong with JNI
     *                       NativeException if the SimGrid native code failed.
     *
     * @see                 Process.restart()
     */
    final static native void processResume(Process process)
	throws JniException, NativeException;
     
    /**
     * The natively implemented method to test if MSG process is suspended.
     *
     * @param process        The valid (binded with a native process) java process to test.
     *
     * @return                If the process is suspended the method retuns true. Otherwise the
     *                        method returns false.
     *
     * @exception            JniException if something goes wrong with JNI
     *
     * @see                 Process.isSuspended()
     */
    final static native boolean processIsSuspended(Process process) throws JniException;
     
    /**
     * The natively implemented method to get the host of a MSG process.
     *
     * @param process        The valid (binded with a native process) java process to get the host.
     *
     * @return                The method returns the host where the process is running.
     *
     * @exception            JniException if something goes wrong with JNI
     *                       NativeException if the SimGrid native code failed.
     *
     * @see                 Process.getHost()
     */
    final static native Host processGetHost(Process process)
	throws JniException, NativeException;
     
    /**
     * The natively implemented method to get a MSG process from his PID.
     *
     * @param PID            The PID of the process to get.
     *
     * @return                The process with the specified PID.
     *
     * @exception            NativeException if the SimGrid native code failed.
     *
     * @see                 Process.getFromPID()
     */
    final static native Process processFromPID(int PID) throws NativeException;
     
    /**
     * The natively implemented method to get the PID of a MSG process.
     *
     * @param process        The valid (binded with a native process) java process to get the PID.
     *
     * @return                The PID of the specified process.
     *
     * @exception            NativeException if the SimGrid native code failed.
     *
     * @see                 Process.getPID()
     */
    final static native int processGetPID(Process process) throws NativeException;
     
    /**
     * The natively implemented method to get the PPID of a MSG process.
     *
     * @param process        The valid (binded with a native process) java process to get the PID.
     *
     * @return                The PPID of the specified process.
     *
     * @exception            NativeException if the SimGrid native code failed.
     *
     * @see                 Process.getPPID()
     */
    final static native int processGetPPID(Process process) throws NativeException;
     
    /**
     * The natively implemented method to get the current running process.
     *
     * @return             The current process.
     *
     * @exception            NativeException if the SimGrid native code failed.

     * @see                Process.currentProcess()
     */
    final static native Process processSelf() throws NativeException;
     
    /**
     * The natively implemented method to get the current running process PID.
     *
     * @return                The PID of the current process.
     *
     * @see                Process.currentProcessPID()
     */
    final static native int processSelfPID();
     
    /**
     * The natively implemented method to get the current running process PPID.
     *
     * @return                The PPID of the current process.
     *
     * @see                Process.currentProcessPPID()
     */
    final static native int processSelfPPID();
     
    /**
     * The natively implemented method to migrate a process from his currnet host to a new host.
     *
     * @param process        The (valid) process to migrate.
     * @param host            A (valid) host where move the process.
     *
     * @exception            JniException if something goes wrong with JNI
     *                       NativeException if the SimGrid native code failed.
     *
     * @see                Process.migrate()
     * @see                Host.getByName()
     */
    final static native void processChangeHost(Process process,Host host) 
	throws JniException, NativeException;
     
    /**
     * Process synchronization. The process wait the signal of the simulator to start.
     * 
     * @exception            JniException if something goes wrong with JNI
     */
    final static native void waitSignal(Process process) throws JniException;
     
    /**
     * The natively implemented native to request the current process to sleep 
     * until time seconds have elapsed.
     *
     * @param seconds        The time the current process must sleep.
     *
     * @exception            NativeException if the SimGrid native code failed.
     *
     * @see                 Process.waitFor()
     */
    final static native void processWaitFor(double seconds) throws NativeException;
     
    /**
     * The natively implemented native method to exit a process.
     *
     * @exception            JniException if something goes wrong with JNI
     *
     * @see                Process.exit()
     */
    final static native void processExit(Process process) throws JniException;
      
     
    /******************************************************************
     * The natively implemented methods connected to the MSG host     *
     ******************************************************************/
     
    /**
     * The natively implemented method to get an host from his name.
     *
     * @param name            The name of the host to get.
     *
     * @return                The host having the specified name.
     *
     * @exception            JniException if something goes wrong with JNI
     *                       HostNotFoundException if there is no such host
     *                       NativeException if the SimGrid native code failed.
     *
     * @see                Host.getByName()
     */
    final static native Host hostGetByName(String name) 
	throws JniException, HostNotFoundException, NativeException;
     
    /**
     * The natively implemented method to get the name of an MSG host.
     *
     * @param host            The host (valid) to get the name.
     *
     * @return                The name of the specified host.
     *
     * @exception            JniException if something goes wrong with JNI
     *
     * @see                Host.getName()
     */
    final static native String hostGetName(Host host) throws JniException;
     
    /**
     * The natively implemented method to get the number of hosts of the simulation.
     *
     * @return                The number of hosts of the simulation.
     *
     * @see                Host.getNumber()
     */
    final static native int hostGetNumber();
     
    /**
     * The natively implemented method to get the host of the current runing process.
     *
     * @return                The host of the current running process.
     *
     * @exception            JniException if something goes wrong with JNI
     *
     * @see                Host.currentHost()
     */
    final static native Host hostSelf() throws JniException;
     
    /**
     * The natively implemented method to get the speed of a MSG host.
     *
     * @param host            The host to get the host.
     *
     * @return                The speed of the specified host.
     *
     * @exception            JniException if something goes wrong with JNI
     *
     * @see                Host.getSpeed()
     */
      
    final static native double hostGetSpeed(Host host) throws JniException;
     
    /**
     * The natively implemented native method to test if an host is avail.
     *
     * @param host            The host to test.
     *
     * @return                If the host is avail the method returns true. 
     *                        Otherwise the method returns false.
     *
     * @exception            JniException if something goes wrong with JNI
     *
     * @see                Host.isAvail()
     */
    final static native boolean hostIsAvail(Host host) throws JniException;
     
    /**
     * The natively implemented native method to get all the hosts of the simulation.
     *
     * @exception            JniException if something goes wrong with JNI
     *
     * @return                A array which contains all the hosts of simulation.
     */
     
    final static native Host[] allHosts() throws JniException;
     
    /**
     * The natively implemented native method to get the number of running tasks on a host.
     *
     * @param                The host concerned by the operation.
     *
     * @return                The number of running tasks.
     *
     * @exception            JniException if something goes wrong with JNI
     *
     */
    final static native int hostGetLoad(Host host) throws JniException;
     
    /******************************************************************
     * The natively implemented methods connected to the MSG task     *
     ******************************************************************/
      
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
     * @param task            The java task object to bind with the native task to create.
     *
     * @exception             JniException if something goes wrong with JNI
     *                        NullPointerException if the specified name is null.
     *                        IllegalArgumentException if compute duration <0 or message size <0
     *
     * @see                    Task.create()
     */
    final static native void taskCreate(Task task,String name,double computeDuration, double messageSize)
	throws JniException, NullPointerException, IllegalArgumentException;
      
    /**
     * The natively implemented method to get the sender of a task.
     *
     * @param    task            The task (valid) to get the sender.
     *
     * @return                The sender of the task.
     *
     * @exception                InvalidTaskException is the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *
     * @see                    Task.getSender()
     */
    final static native Process taskGetSender(Task task) throws JniException;
      
    /**
     * The natively implementd method to get the source of a task.
     *
     * @param task            The task to get the source.
     *
     * @return                The source of the task.
     *
     * @exception                InvalidTaskException is the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *
     * @see                    Task.getSource()
     */
    final static native Host taskGetSource(Task task) throws JniException, NativeException;
      
    /**
     * The natively implemented method to get the name of the task.
     *
     * @param task            The task to get the name.
     *
     * @return                 The name of the specified task.
     *
     * @exception                InvalidTaskException is the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *
     * @see                    Task.getName()
     */
    final static native String taskGetName(Task task) throws JniException;
      
    /**
     * The natively implemented method to cancel a task.
     *
     * @param task            The task to cancel.
     *
     * @exception                InvalidTaskException if the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *                        MsgException if the cancelation failed.
     *
     * @see                    Task.cancel().
     */
    final static native void taskCancel(Task task) throws JniException, NativeException;
      
    /**
     * The natively implemented method to get the computing amount of the task.
     *
     * @param task            The task to get the computing amount.
     *
     * @return                The computing amount of the specified task.
     *
     * @exception                InvalidTaskException if the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *                        MsgException if the cancelation failed.
     *
     * @see                    Task.getComputeDuration()
     */
    final static native double taskGetComputeDuration(Task task) throws JniException;
      
    /**
     * The natively implemented method to get the remaining computation
     *
     * @param task            The task to get the remaining computation.
     *
     * @return                The remaining computation of the specified task.
     *
     * @exception                InvalidTaskException if the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *                        MsgException if the cancelation failed.
     *
     * @see                    Task.getRemainingDuration()
     */
    final static native double taskGetRemainingDuration(Task task) throws JniException;
      
    /**
     * The natively implemented method to set the priority of a task.
     *
     * @param task            The task to set the prirotity
     *
     * @param priority        The new priority of the specified task.
     *
     * @exception                InvalidTaskException if the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *                        MsgException if the cancelation failed.
     *
     * @see                    Task.setPriority()
     */
    final static native void taskSetPriority(Task task,double priority) throws JniException;
      
    /**
     * The natively implemented method to destroy a MSG task.
     *
     * @param                    The task to destroy.
     *
     * @exception                InvalidTaskException is the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *                        MsgException if the destruction failed.
     *
     * @see                    Task.destroy()
     */
    final static native void taskDestroy(Task task) throws JniException, NativeException;
      
    /**
     * The natively implemented method to execute a MSG task.
     *
     * @param task            The task to execute.
     *
     * @exception                InvalidTaskException is the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *                        MsgException if the destruction failed.
     *
     * @see                    Task.execute()
     */
    final static native void taskExecute(Task task) throws JniException, NativeException;
      
      
     
    /**************************************************************************
     * The natively implemented methods connected to the MSG parallel task     *
     ***************************************************************************/
      
    /**
     * The natively implemented method to create a MSG parallel task.
     *
     * @param name                The name of the parallel task.
     * @param hosts                The list of hosts implied by the parallel task.
     * @param computeDurations    The total number of operations that have to be performed
     *                            on the hosts.
     * @param messageSizes        An array of doubles
     *
     * @see                        ParallelTask.create()
     */
    final static native void parallelTaskCreate(ParallelTask parallelTask, String name, 
						       Host[] hosts, double[] computeDurations, double[] messageSizes)
	throws JniException, NullPointerException, IllegalArgumentException;
      
    /**
     * The natively implemented method to get the sender of a parallel task.
     *
     * @param    parallelTask    The parallel task (valid) to get the sender.
     *
     * @return                The sender of the parallel task.
     *
     * @see                    ParallelTask.getSender()
     */
    final static native Process parallelTaskGetSender(ParallelTask parallelTask) throws JniException;
      
    /**
     * The natively implementd method to get the source of a parallel task.
     *
     * @param parallelTask    The parallel task to get the source.
     *
     * @return                The source of the parallel task.
     *
     * @see                    ParallelTask.getSource()
     */
    final static native Host parallelTaskGetSource(ParallelTask parallelTask) throws JniException;
      
    /**
     * The natively implemented method to get the name of the parallel task.
     *
     * @param parallelTask    The parallel task to get the name.
     *
     * @return                 The name of the specified parallel task.
     *
     * @see                    ParallelTask.getName()
     */
    final static native String parallelTaskGetName(ParallelTask parallelTask) throws JniException;
      
    /**
     * The natively implemented method to cancel a parallel task.
     *
     * @param parallelTask    The parallel task to cancel.
     *
     * @see                    ParallelTask.cancel().
     */
    final static native void parallelTaskCancel(ParallelTask parallelTask) throws JniException,NativeException;
      
    /**
     * The natively implemented method to get the computing amount of the task.
     *
     * @param parallelTask    The parallel task to get the computing amount.
     *
     * @return                The computing amount of the specified parallel task.
     *
     * @see                    ParallelTask.getComputeDuration()
     */
    final static native double parallelTaskGetComputeDuration(ParallelTask parallelTask) throws JniException;
      
    /**
     * The natively implemented method to get the remaining computation
     *
     * @param parallelTask    The parallel task to get the remaining computation.
     *
     * @return                The remaining computation of the specified parallel task.
     *
     * @see                    ParallelTask.getRemainingDuration()
     */
    final static native double parallelTaskGetRemainingDuration(ParallelTask parallelTask) throws JniException;
      
    /**
     * The natively implemented method to set the priority of a parallel task.
     *
     * @param parallelTask    The parallel task to set the prirotity
     *
     * @param priority        The new priority of the specified parallel task.
     *
     * @see                    ParallelTask.setPriority()
     */
    final static native void parallelTaskSetPriority(ParallelTask parallelTask,double priority) throws JniException;
      
    /**
     * The natively implemented method to destroy a MSG parallel task.
     *
     * @param parallelTask    The parallel task to destroy.
     *
     * @see                    ParallelTask.destroy()
     */
    final static native void parallelTaskDestroy(ParallelTask parallelTask) throws JniException,NativeException;
      
    /**
     * The natively implemented method to execute a MSG parallel task.
     *
     * @param parallelTask    The parallel task to execute.
     *
     * @see                    ParallelTask.execute()
     */
    final static native void parallelTaskExecute(ParallelTask parallelTask) throws JniException, NativeException;
      
    /******************************************************************
     * The natively implemented methods connected to the MSG channel  *
     ******************************************************************/
      
    /**
     * The natively implemented method to listen on the channel and wait for receiving a task.
     *
     * @param channel            The channel to listen            
     *
     * @return                The task readed from the channel.
     *
     * @exception                MsgException if the listening operation failed.
     *
     * @see                    Channel.get()
     */
    final static native Task channelGet(Channel channel) throws JniException,NativeException;
      
    /**
     * The natively implemented method to listen on the channel and wait for receiving a task with a timeout.
     *
     * @param channel            The channel to listen.
     * @param timeout            The timeout of the listening.
     *
     * @return                The task readed from the channel.
     *
     * @exception                MsgException if the listening operation failed.
     *
     * @see                    Channel.getWithTimeout()
     *
     */ 
    final static native Task channelGetWithTimeout(Channel channel,double timeout) throws JniException,NativeException;
      
      
    /**
     * The natively implemented method to listen on the channel of a specific host.
     *
     * @param    channel            The channel to listen.
     *
     * @param host            The specific host.
     *
     * @return                The task readed from the channel of the specific host.
     *
     * @exception                InvalidHostException if the specified host is not valid.
     *                        MsgException if the listening operation failed.
     *
     * @see                    Channel.getFromHost()
     */    
    final static native Task channelGetFromHost(Channel channel,Host host) throws JniException,NativeException;
      
    /**
     * The natively implemented method to test whether there is a pending communication on the channel.
     *
     * @param channel            The channel concerned by the operation.
     *
     * @return                The method returns true if there is a pending communication on the specified
     *                        channel. Otherwise the method returns false.
     *
     * @see                    Channel.hasPendingCommunication()
     */                
    final static native boolean channelHasPendingCommunication(Channel channel) throws JniException;
      
    /**
     * The natively implemented method to test whether there is a pending communication on a 
     * channel, and who sent it.
     *
     * @param                    The channel concerned by the operation.
     *
     * @return                The method returns -1 if there is no pending communication and 
     *                        the PID of the process who sent it otherwise.
     *
     * @see                    Channel.getCummunicatingProcess()
     */
    final static native int channelGetCommunicatingProcess(Channel channel) throws JniException;
      
    /**
     * The natively implemented method to get the number of tasks waiting to be received on a
     * channel and sent by a host.
     *
     * @param channel            The channel concerned by the operation.
     *
     * @param host            The sender of the tasks.
     *
     * @return                The number of tasks waiting to be received on a channel
     *                         and sent by the specified host.
     *
     * @exception                InvalidHostException if the specified host is not valid.
     *
     * @see                    Channel.getHostWaiting()
     */
    final static native int channelGetHostWaitingTasks(Channel channel,Host host) throws JniException;
      
    /**
     * The natively implemented method to put a task on the channel of an host.
     *
     * @param channel            The channel where to put the task.
     * @param task            The task to put.
     * @param host            The host of the specified channel.
     *
     * @exception                InvalidTaskException if the task is not valid.
     *                        InvalidHostException if the host is not valid.
     *                        MsgException if the operation failed.
     *
     * @see                    Channel.put()
     */                
    final static native void channelPut(Channel channel,Task task,Host host) throws JniException,NativeException;
      
    /**
     * The natively implemented method to put a task on a channel of an  host (with a timeout 
     * on the waiting of the destination host) and waits for the end of the transmission.
     *
     * @param channel            The channel where to put the task.
     * @param task            The task to put.
     * @param host            The host containing the channel.
     * @param timeout            The timeout of the transmission.
     *
     * @exception                InvalidTaskException if the task is not valid.
     *                        InvalidHostException if the host is not valid.
     *                        MsgException if the operation failed.
     *
     * @see                    Channel.putWithTimeout()
     */
    final static native void channelPutWithTimeout(Channel channel,Task task,Host host,double timeout) throws JniException,NativeException;
      
    /**
     * The natively implemented method to put a task on channel with a bounded transmition
     * rate.
     * 
     * @param channel            The channel where to put the task.
     * @param task            The task to put.
     * @param host            The host containing the channel.
     * @param max_rate        The bounded transmition rate.
     *
     * @exception                InvalidTaskException if the task is not valid.
     *                        InvalidHostException if the host is not valid.
     *                        MsgException if the operation failed.
     *
     * @see                    Channel.putBounded()
     */
    final static native void channelPutBounded(Channel channel,Task task,Host host,double max_rate) throws JniException,NativeException;
      
    /**
     * The natively implemented method to wait for at most timeout seconds for a task reception
     * on channel. The PID is updated with the PID of the first process.
     *
     * @param channel            The channel concerned by the operation.
     * @param timeout            The maximum time to wait for a task before
     *                         giving up.
     *
     * @exception                MsgException if the reception operation failed.
     *
     * @see                    Channel.wait()
     */
    final static native int channelWait(Channel channel, double timeout) throws JniException,NativeException;
      
    /**
     * The natively implemented method to set the number of channel used by all the process
     * of the simulation.
     *
     * @param channelNumber    The channel number of the process.
     *
     * @see                    Channel.setNumber()
     */
    final static native void channelSetNumber(int channelNumber);
      
    /**
     * The natively implemented method to get the number of channel of the process of the simulation.
     *
     * @return                The number of channel per process.
     *
     * @see                    Channel.getNumber()
     */
    final static native int channelGetNumber();
             
    /*********************************************************************************
     * Additional native methods                                                      *
     **********************************************************************************/
      
    /**
     * The natively implemented method to get the simulation time.
     *
     * @param                    The simulation time.
     */
    public final static native double getClock();
      
    public final static native void pajeOutput(String pajeFile);
      
       
    public final static native void info(String s);
                        
    /*********************************************************************
     * The natively implemented methods connected to the MSG simulation  *
     *********************************************************************/
       
    /**
     * The natively implemented method to initialize a MSG simulation.
     *
     * @param args            The arguments of the command line of the simulation.
     *
     * @see                    Msg.init()
     */
    public final static native void init(String[] args);
       
    /**
     * Run the MSG simulation, and cleanup everything afterward.
     *
     * If you want to chain simulations in the same process, you
     * should call again createEnvironment and deployApplication afterward.
     *
     * @see                    MSG_run, MSG_clean
     */
    public final static native void run() throws NativeException;
       
    /**
     * The native implemented method to create the environment of the simulation.
     *
     * @param platformFile    The XML file which contains the description of the environment of the simulation
     *
     */
    public final static native void createEnvironment(String platformFile) throws NativeException;
       
       
    /**
     * The method to deploy the simulation.
     *
     * @param appFile        The XML file which contains the description of the application to deploy.
     */
       
        
    public static void deployApplication(String platformFile) {
	try {
	    Class c = Class.forName("com.sun.org.apache.xerces.internal.parsers.SAXParser");
	    XMLReader reader = (XMLReader)c.newInstance();
	    reader.setEntityResolver(new DTDResolver());
	    ApplicationHandler handler = new ApplicationHandler();
	    reader.setContentHandler(handler);
	    reader.setFeature("http://xml.org/sax/features/validation", false);
	    reader.parse(platformFile);

	} catch(Exception e) {
	    /* FIXME: do not swallow exception ! */
	    System.out.println("Exception in Msg.launchApplication()");
	    System.out.println(e);
	    e.printStackTrace();
	}        
    }                    

    /* The launcher */
    static public void main(String[]args) throws MsgException {
	/* initialize the MSG simulation. Must be done before anything else (even logging). */
	Msg.init(args);

	if(args.length < 2) {
    		
	    Msg.info("Usage: Msg platform_file deployment_file");
	    System.exit(1);
	}
		
	/* specify the number of channel for the process of the simulation. */
	Channel.setNumber(1);
		
	/* Load the platform and deploy the application */
	Msg.createEnvironment(args[0]);
	Msg.deployApplication(args[1]);
		
	/* Execute the simulation */
	Msg.run();
    }
}
