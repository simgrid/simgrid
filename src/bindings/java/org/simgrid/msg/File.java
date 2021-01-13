/* Copyright (c) 2012-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

public class File {
	public static final int SEEK_SET = 0;
	public static final int SEEK_CUR = 1;
	public static final int SEEK_END = 2;
	/**
	 * Represents the bind between the java comm and the
	 * native C comm. You must never access it, since it is
	 * automatically set.
	 */
	private long bind = 0;
	/**
	 * Constructor, opens the file.
	 * @param path is the file location on the storage
	 */
	public File(String path) {
		open(path);
	}

	/**
	 * Opens the file whose name is the string pointed to by path.
	 * @param path is the file location on the storage
	 */
	protected native void open(String path);
	/**
	 * Read elements of a file.
	 * @param size of each element
	 * @param nMemb is the number of elements of data to write
	 * @return the actually read size
	 */
	public native long read(long size, long nMemb);

	/**
	 * Write elements into a file.
	 * @param size of each element
	 * @param nMemb is the number of elements of data to write
	 * @return the actually written size
	 */
	public native long write(long size, long nMemb);
	/**
	 * Write elements into a file.
	 * @param offset : number of bytes to offset from origin
	 * @param origin : Position used as reference for the offset. It is specified by one of the following constants
	 *                 defined in &lt;stdio.h&gt; exclusively to be used as arguments for this function (SEEK_SET =
	 *                 beginning of file, SEEK_CUR = current position of the file pointer, SEEK_END = end of file)
 	 */
	public native void seek(long offset, long origin);

	/** Close the file. */
	public native void close();

	/** Class initializer, to initialize various JNI stuff */
	public static native void nativeInit();
	static {
		org.simgrid.NativeLib.nativeInit();
		nativeInit();
	}	
}