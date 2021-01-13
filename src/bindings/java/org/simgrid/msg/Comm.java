/* Copyright (c) 2012-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/** Communication action, representing an ongoing communication between processes. */
public class Comm {
	/** Indicates if the communication is a receiving communication */
	protected boolean receiving;
	/** Indicates if the communication is finished */
	protected boolean finished = false;
	/**
	 * Represents the bind between the java comm and the
	 * native C comm. You must never access it, since it is
	 * automatically set.
	 */
	private long bind = 0;
	/** Represents the bind for the task object pointer. Don't touch it. */
	private long taskBind = 0;
	/** Task associated with the comm. Beware, it can be null */
	protected Task task = null;
	/**
	 * Protected constructor, used by Comm factories
	 * in Task.
	 */
	protected Comm() {

	}
	/**
	 * Destroy the C communication object, when the GC reclaims the java part.
	 * @deprecated (from Java9 onwards)
	 */
	@Deprecated @Override
	protected void finalize() throws Throwable{
		nativeFinalize();
	}
	protected native void nativeFinalize();
	/**
	 * Returns if the communication is finished or not.
	 * If the communication has finished and there was an error,
	 * raise an exception.
	 */
	public native boolean test() throws TransferFailureException, HostFailureException, TimeoutException ;
	/** Wait infinitely for the completion of the communication (infinite timeout) */
	public void waitCompletion() throws TransferFailureException, HostFailureException, TimeoutException {
		waitCompletion(-1);
	}
	/**
	 * Wait for the completion of the communication.
	 * Throws an exception if there were an error in the communication.
	 * @param timeout Time before giving up (infinite time if negative)
	 */
	public native void waitCompletion(double timeout) throws TransferFailureException, HostFailureException, TimeoutException;

	/** Wait all of the communications */
	public static native void waitAll(Comm[] comms, double timeout) throws TransferFailureException, HostFailureException, TimeoutException;
	/** Wait all of the communications, with no maximal delay */
	public static void waitAll(Comm[] comms) throws TransferFailureException, HostFailureException, TimeoutException {
		waitAll(comms, -1.);
	}
	/** Wait any of the communications, and return the rank of the terminating comm */
	public static native int waitAny(Comm[] comms) throws TransferFailureException, HostFailureException, TimeoutException;
	/**
	 * Returns the task associated with the communication.
	 * if the communication isn't finished yet, will return null.
	 */
	public Task getTask() {
		return task;
	}

	/** Class initializer, to initialize various JNI stuff */
	public static native void nativeInit();
	static {
		org.simgrid.NativeLib.nativeInit();
		nativeInit();
	}	
}
