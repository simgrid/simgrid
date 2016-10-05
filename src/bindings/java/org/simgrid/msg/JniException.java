/* This exception is raised when there is a problem within the bindings (in JNI). */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/**
 * This exception is raised when there is a problem within the bindings (in JNI). 
 * That's a RuntimeException: I guess nobody wants to survive a JNI error in SimGrid
 */
public class JniException extends RuntimeException {
	private static final long serialVersionUID = 1L;


	/** Constructs an <code>JniException</code> without a detail message. */
	public JniException() {
		super();
	}
	/** Constructs an <code>JniException</code> with a detail message. */ 
	public JniException(String s) {
		 super(s);
	}
	public JniException(String string, Exception e) {
		super(string,e);
	}
}
