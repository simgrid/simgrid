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

/* Contains all the native methods reloaded by Process, Host and Task.
 *
 * @author  Abdelmalek Cherier
 * @author  Martin Quinson
 * @since SimGrid 3.3
 */
final class MsgNative {


  final static native
    void selectContextFactory(String name) throws NativeException;

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
  final static native boolean processIsSuspended(Process process) throws
    JniException;

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
  final static native int processGetPID(Process process) throws
    NativeException;

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
  final static native int processGetPPID(Process process) throws
    NativeException;

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
  final static native void processChangeHost(Process process, Host host)
  throws JniException, NativeException;

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
  final static native void processWaitFor(double seconds) throws
    NativeException;

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
  final static native void taskCreate(Task task, String name,
                                      double computeDuration,
                                      double messageSize)
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
  final static native Host taskGetSource(Task task) throws JniException,
    NativeException;

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
  final static native void taskCancel(Task task) throws JniException,
    NativeException;

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
  final static native void parallelTaskCreate(Task pTask, String name,
                                              Host[]hosts,
                                              double[]computeDurations,
                                              double[]messageSizes)
  throws JniException, NullPointerException, IllegalArgumentException;

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
  final static native double taskGetComputeDuration(Task task) throws
    JniException;

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
  final static native double taskGetRemainingDuration(Task task) throws
    JniException;

    /**
     * The natively implemented method to set the priority of a task.
     *
     * @param task            The task to set the priority
     *
     * @param priority        The new priority of the specified task.
     *
     * @exception                InvalidTaskException if the specified task is not valid. A task
     *                        is invalid if it is not binded with a native task.
     *                        MsgException if the cancelation failed.
     *
     * @see                    Task.setPriority()
     */
  final static native void taskSetPriority(Task task,
                                           double priority) throws
    JniException;

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
  final static native void taskDestroy(Task task) throws JniException,
    NativeException;

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
  final static native void taskExecute(Task task) throws JniException,
    NativeException;


    /******************************************************************
     * Task reception methods                                         *
     ******************************************************************/

    /**
     * The natively implemented method to listen on the channel and wait for receiving a task with a timeout.
     *
     * @param channel  The channel to listen.
     * @param timeout  The timeout of the listening. 
     * @param host     The specific host from which we accept messages (or NULL for any).
     *
     * @return         The task read from the channel.
     *
     * @exception                MsgException if the listening operation failed.
     *
     * @see                    Channel.getWithTimeout()
     *
     */
  final static native Task taskGet(int channel, double timeout,
                                   Host host) throws JniException,
    NativeException;


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
  final static native boolean taskProbe(int channel) throws JniException;

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
  final static native int taskGetCommunicatingProcess(int channel) throws
    JniException;

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
  final static native int taskProbeHost(int channel,
                                        Host host) throws JniException;

    /******************************************************************
     * Task emission methods                                          *
     ******************************************************************/
    /**
     * The natively implemented method to put a task on the channel of an host.
     *
     * @param host            The host on which to send the data
     * @param channel         The channel where to put the task.
     * @param task            The task to put.
     * @param task            Timeout (or <0 if none)
     *
     * @exception                InvalidTaskException if the task is not valid.
     *                        InvalidHostException if the host is not valid.
     *                        MsgException if the operation failed.
     *
     */
  final static native void hostPut(Host host, int channel, Task task,
                                   double timeout) throws JniException,
    NativeException;


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
  final static native void hostPutBounded(Host host, int channel, Task task,
                                          double max_rate) throws
    JniException, NativeException;

}
