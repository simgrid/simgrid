/*
 * Contains all the native methods related to Process, Host and Task.
 *
 * Copyright 2006,2007,2010 The SimGrid Team           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */

package org.simgrid.msg;

import org.simgrid.msg.Process;

/* FIXME: split into internal classes of Msg, Task, Host etc. */

/**
 *  Contains all the native methods related to Process, Host and Task.
 */
final class MsgNative {

	/******************************************************************
	 * The natively implemented methods connected to the MSG Process  *
	 ******************************************************************/
	/**
	 * The natively implemented method to create an MSG process.
	 *
	 * @param process The java process object to bind with the MSG native process.
	 * @param host    A valid (binded) host where create the process.
	 *
	 * @see  Process constructors.
	 */
	final static native
	void processCreate(Process process, String hostName) throws HostNotFoundException;

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
	 * @see                 Process.pause()
	 */
	final static native void processSuspend(Process process);

	/**
	 * The natively implemented method to kill a MSG process.
	 *
	 * @param process        The valid (binded with a native process) java process to kill.
	 *
	 * @see                 Process.kill()
	 */
	final static native void processKill(Process process);

	/**
	 * The natively implemented method to resume a suspended MSG process.
	 *
	 * @param process        The valid (binded with a native process) java process to resume.
	 *
	 *
	 * @see                 Process.restart()
	 */
	final static native void processResume(Process process);

	/**
	 * The natively implemented method to test if MSG process is suspended.
	 *
	 * @param process        The valid (binded with a native process) java process to test.
	 *
	 * @return                If the process is suspended the method retuns true. Otherwise the
	 *                        method returns false.
	 *
	 * @see                 Process.isSuspended()
	 */
	final static native boolean processIsSuspended(Process process);

	/**
	 * The natively implemented method to get the host of a MSG process.
	 *
	 * @param process        The valid (binded with a native process) java process to get the host.
	 *
	 * @return                The method returns the host where the process is running.
	 *
	 * @exception            HostNotFoundException if the SimGrid native code failed (initialization error?).
	 *
	 * @see                 Process.getHost()
	 */
	final static native Host processGetHost(Process process);

	/**
	 * The natively implemented method to get a MSG process from his PID.
	 *
	 * @param PID            The PID of the process to get.
	 *
	 * @return                The process with the specified PID.
	 *
	 * @see                 Process.getFromPID()
	 */
	final static native Process processFromPID(int PID) ;

	/**
	 * The natively implemented method to get the PID of a MSG process.
	 *
	 * @param process        The valid (binded with a native process) java process to get the PID.
	 *
	 * @return                The PID of the specified process.
	 *
	 * @see                 Process.getPID()
	 */
	final static native int processGetPID(Process process);

	/**
	 * The natively implemented method to get the PPID of a MSG process.
	 *
	 * @param process        The valid (binded with a native process) java process to get the PID.
	 *
	 * @return                The PPID of the specified process.
	 *
	 * @see                 Process.getPPID()
	 */
	final static native int processGetPPID(Process process);

	/**
	 * The natively implemented method to get the current running process.
	 *
	 * @return             The current process.
	 *
	 * @see                Process.currentProcess()
	 */
	final static native Process processSelf();

	/**
	 * The natively implemented method to migrate a process from his currnet host to a new host.
	 *
	 * @param process        The (valid) process to migrate.
	 * @param host            A (valid) host where move the process.
	 *
	 *
	 * @see                Process.migrate()
	 * @see                Host.getByName()
	 */
	final static native void processMigrate(Process process, Host host) ;

	/**
	 * The natively implemented native to request the current process to sleep 
	 * until time seconds have elapsed.
	 *
	 * @param seconds        The time the current process must sleep.
	 *
	 * @exception            HostFailureException if the SimGrid native code failed.
	 *
	 * @see                 Process.waitFor()
	 */
	final static native void processWaitFor(double seconds) throws HostFailureException;

	/**
	 * The natively implemented native method to exit a process.
	 *
	 * @see                Process.exit()
	 */
	final static native void processExit(Process process);


	/******************************************************************
	 * The natively implemented methods connected to the MSG host     *
	 ******************************************************************/

	/**
	 * The natively implemented native method to get all the hosts of the simulation.
	 *
	 * @return                A array which contains all the hosts of simulation.
	 */

	final static native Host[] allHosts();

}
