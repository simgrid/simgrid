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
	 * The natively implemented method to get an host from his name.
	 *
	 * @param name            The name of the host to get.
	 *
	 * @return                The host having the specified name.
	 *
	 * @exception            HostNotFoundException if there is no such host
	 *                       
	 *
	 * @see                Host.getByName()
	 */
	final static native Host hostGetByName(String name) throws HostNotFoundException;

	/**
	 * The natively implemented method to get the name of an MSG host.
	 *
	 * @param host            The host (valid) to get the name.
	 *
	 * @return                The name of the specified host.
	 *
	 * @see                Host.getName()
	 */
	final static native String hostGetName(Host host);

	/**
	 * The natively implemented method to get the number of hosts of the simulation.
	 *
	 * @return                The number of hosts of the simulation.
	 *
	 * @see                Host.getNumber()
	 */
	final static native int hostGetCount();

	/**
	 * The natively implemented method to get the host of the current runing process.
	 *
	 * @return                The host of the current running process.
	 *
	 * @see                Host.currentHost()
	 */
	final static native Host hostSelf();

	/**
	 * The natively implemented method to get the speed of a MSG host.
	 *
	 * @param host            The host to get the host.
	 *
	 * @return                The speed of the specified host.
	 *
	 * @see                Host.getSpeed()
	 */

	final static native double hostGetSpeed(Host host);

	/**
	 * The natively implemented native method to test if an host is avail.
	 *
	 * @param host            The host to test.
	 *
	 * @return                If the host is avail the method returns true. 
	 *                        Otherwise the method returns false.
	 *
	 * @see                Host.isAvail()
	 */
	final static native boolean hostIsAvail(Host host);

	/**
	 * The natively implemented native method to get all the hosts of the simulation.
	 *
	 * @return                A array which contains all the hosts of simulation.
	 */

	final static native Host[] allHosts();

	/**
	 * The natively implemented native method to get the number of running tasks on a host.
	 *
	 * @param                The host concerned by the operation.
	 *
	 * @return                The number of running tasks.
	 */
	final static native int hostGetLoad(Host host);
}
