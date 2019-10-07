/* JNI interface to C RngStream code */

/* Copyright (c) 2006-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;
/**
 * Export of RngStreams for Java
 */
public class RngStream {
	/**
	 * Represents the bind between the RngStream java object and the C object.
	 */
	private long bind;
	/**
	 * Creates and returns a new stream without identifier.
	 * This procedure reserves space to keep the information relative to
	 * the RngStream, initializes its seed Ig , sets Bg and Cg equal to Ig , sets its antithetic and
	 * precision switches to 0. The seed Ig is equal to the initial seed of the package given by
	 * setPackageSeed if this is the first stream created, otherwise it is Z steps ahead
	 * of that of the most recently created stream.
	 */
	public RngStream() {
		create("");
	}
	/**
	 * Creates and returns a new stream with identifier "name".
	 * This procedure reserves space to keep the information relative to
	 * the RngStream, initializes its seed Ig , sets Bg and Cg equal to Ig , sets its antithetic and
	 * precision switches to 0. The seed Ig is equal to the initial seed of the package given by
	 * setPackageSeed if this is the first stream created, otherwise it is Z steps ahead
	 * of that of the most recently created stream.
	 */
	public RngStream(String name) {
		create(name);
	}
	/**
	 * The natively implemented method to create a C RngStream object.
	 */
	private native void create(String name);

	/** @deprecated (from Java9 onwards) */
	@Deprecated @Override
	protected void finalize() throws Throwable{
		nativeFinalize();
	}
	/**
	 * Release the C RngStream object
	 */
	private native void nativeFinalize();

	/**
	 * Sets the initial seed of the package RngStreams to the six integers in the vector seed. This will
	 * be the seed (initial state) of the first stream. If this procedure is not called, the default initial
	 * seed is (12345, 12345, 12345, 12345, 12345, 12345). If it is called, the first 3 values of the seed
	 * must all be less than m1 = 4294967087, and not all 0; and the last 3 values must all be less
	 * than m2 = 4294944443, and not all 0. Returns false for invalid seeds, and true otherwise.
	 */
	public static native boolean setPackageSeed(int[] seed);
	/**
	 * Reinitializes the stream g to its initial state: Cg and Bg are set to Ig .
	 */
	public native void resetStart();
	/**
	 * Reinitializes the stream g to the beginning of its current substream: Cg is set to Bg .
	 */
	public native void restartStartSubstream();
	/**
	 * Reinitializes the stream g to the beginning of its next substream: Ng is computed, and Cg and
	 * Bg are set to Ng .
	 */
	public native void resetNextSubstream();
	/**
	 * If a = true the stream g will start generating antithetic variates, i.e., 1 - U instead of U , until
	 *  this method is called again with a = false.
	 */
	public native void setAntithetic(boolean a);
	/**
	 * Sets the initial seed Ig of stream g to the vector seed. This vector must satisfy the same
	 * conditions as in setPackageSeed. The stream is then reset to this initial seed. The
	 * states and seeds of the other streams are not modified. As a result, after calling this procedure,
	 * the initial seeds of the streams are no longer spaced Z values apart. We discourage the use of
	 * this procedure. Returns false for invalid seeds, and true otherwise.
	 */
	public native boolean setSeed(int[] seed);
	/**
	 * Advances the state of the stream by k values, without modifying the states of other streams (as
	 * in RngStream_SetSeed), nor the values of Bg and Ig associated with this stream. If e &gt; 0, then
	 * k = 2e + c; if e &lt; 0, then k = -2-e + c; and if e = 0, then k = c. Note: c is allowed to take
	 * negative values. We discourage the use of this procedure.	
	 */
	public native void advanceState(int e, int g);

	/**
	 * Returns a (pseudo)random number from the uniform distribution over the interval (0, 1), after advancing the state by one step. The returned number has 32 bits of precision
	 * in the sense that it is always a multiple of 1/(232 - 208), unless RngStream_IncreasedPrecis
	 * has been called for this stream.
	 */
	public native double randU01();
	/**
	 * Returns a (pseudo)random number from the discrete uniform distribution over the integers
	 * {i, i + 1, . . . , j}
	 */
	public native int randInt(int i, int j);

	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		org.simgrid.NativeLib.nativeInit();
		nativeInit();
	}
}
