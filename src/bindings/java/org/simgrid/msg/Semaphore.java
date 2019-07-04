/* Copyright (c) 2012-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;
/** A semaphore implemented on top of SimGrid synchronization mechanisms.
 * You can use it exactly the same way that you use classical semaphores
 * but to handle the interactions between the processes within the simulation.
 *
 */

public class Semaphore {
	private long bind; // The C object -- don't touch it
	/**
	 * Semaphore capacity, defined when the semaphore is created. At most capacity
	 * process can acquire this semaphore at the same time.
	 */
	protected final int capacity;
	/**
	 * Creates a new semaphore with the given capacity. At most capacity
	 * process can acquire this semaphore at the same time.
	 */
	public Semaphore(int capacity) {
		init(capacity);
		this.capacity = capacity;
	}
	/** The native implementation of semaphore initialization
	 */
	private native void init(int capacity);


	/** Locks on the semaphore object until the provided timeout expires
	 * @exception TimeoutException if the timeout expired before
	 *            the semaphore could be acquired.
	 * @param timeout the duration of the lock
	 */
	public native void acquire(double timeout) throws TimeoutException;
	/** Locks on the semaphore object with no timeout
	 */
	public void acquire() {
		try {
			acquire(-1);
		} catch (TimeoutException e) {
			e.printStackTrace(); // This should not happen.
			assert false ;
		}
	}
	/** Releases the semaphore object
	 */
	public native void release();
	/** returns a boolean indicating it this semaphore would block at this very specific time
	 *
	 * Note that the returned value may be wrong right after the
	 * function call, when you try to use it...  But that's a
	 * classical semaphore issue, and SimGrid's semaphores are not
	 * different to usual ones here.
	 */
	public native boolean wouldBlock();

	/** Returns the semaphore capacity
	 */
	public int getCapacity(){
		return this.capacity;
	}


	/**
	 * Deletes this semaphore when the GC reclaims it
	 * @deprecated (from Java9 onwards)
	 */
	@Deprecated @Override
	protected void finalize() throws Throwable {
		nativeFinalize();
	}
	private native void nativeFinalize();
	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		org.simgrid.NativeLib.nativeInit();
		nativeInit();
	}
}
